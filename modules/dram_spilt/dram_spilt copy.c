/*
 * dram_realloc_full_4_1_15.c
 *
 * 适用于 Linux 4.1.15 的示例模块：对 DRAM 进行动态分割和重新分配 (reallocate)，
 * 并提供 /proc 接口用于调试：
 *
 *   - /proc/dram_realloc_demo/dram_info   ：read-only，查看当前 dram 信息
 *   - /proc/dram_realloc_demo/dram_realloc：write-only，写入新的 OS
 * 管理区大小（MB）以触发 reallocate
 *
 * 主要内容：
 *  1. 全局 dram_info 结构体定义与初始化
 *  2. do_dram_reallocate() 核心逻辑（仅支持“扩大”场景，收缩返回 -EINVAL）
 *  3. /proc/dram_info 读接口（使用 seq_file）
 *  4. /proc/dram_realloc 写接口
 *  5. module_init/module_exit
 *
 * 编译测试：在 Linux 4.1.15 源码树下，放到 drivers/misc/，并在相应 Makefile
 * 添加： obj-m += dram_realloc_full_4_1_15.o
 *
 * 然后在外部模块环境（如 Buildroot/Yocto/手工 Makefile）里：
 *   make -C /path/to/linux-4.1.15 M=$PWD modules
 *
 * 加载后，可通过：
 *   cat /proc/dram_realloc_demo/dram_info
 *   echo 768 > /proc/dram_realloc_demo/dram_realloc
 *
 * 注意：本示例默认将总内存的 25% 交给 OS 管理，起始地址硬编码为
 * 0x3000_0000，仅作演示； 如果要在实际平台使用，请根据 Device Tree/Platform
 * Data 获取真实的 os_start_addr。
 */

#include <linux/errno.h>
#include <linux/highmem.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/memblock.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/printk.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/uaccess.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("DRAM Reallocate with /proc Interface (Linux 4.1.15)");
MODULE_VERSION("1.0");

extern void free_bootmem_late(unsigned long addr, unsigned long size);
extern int memblock_free(phys_addr_t base, phys_addr_t size);
extern int memblock_remove(phys_addr_t base, phys_addr_t size);
#define PFX "[dram_realloc] "

/* DRAM 分割标志 */
#define DRAM_SPLIT_NOT_SUPPORTED 0
#define DRAM_SPLIT_SUPPORTED 1
#define DRAM_SPLIT_ALREADY 2

/* 按 a 对 x 向上对齐 */
#ifndef ALIGN
#define ALIGN(x, a) (((x) + ((a)-1)) & ~((a)-1))
#endif

/*
 * 全局结构：维护 DRAM 总大小以及 OS 管理的区域
 *  - total_dram       : 物理总内存，单位 MB
 *  - os_managed_dram  : 操作系统当前正在管理的内存大小（MB）
 *  - default_os_dram  : 初始化时 OS 默认管理内存大小（MB）
 *  - os_start_addr    : OS 管理区域的起始物理地址（字节）
 *  - dyn_split_flag   : 分割状态标志（NOT_SUPPORTED / SUPPORTED / ALREADY）
 */
struct dram_info {
  struct mutex lock;
  unsigned long total_dram;      /* 单位：MB */
  unsigned long os_managed_dram; /* 单位：MB */
  unsigned long default_os_dram; /* 单位：MB */
  unsigned long os_start_addr;   /* 起始物理地址 (字节) */
  int dyn_split_flag;
};

/* 全局实例 */
static struct dram_info g_curt_dram;

/**
 * init_dram_info() - 初始化全局 DRAM 信息
 *
 * - 自动从 totalram_pages 计算出当前可用的物理总内存（MB）；
 * - 默认让操作系统管理总内存的 25%（可根据实际情况调整）；
 * - 设置 OS 管理区的起始地址（示例写死为 0x30000000，实际平台应从 DT/Platform
 * Data 获取）；
 * - 将 dyn_split_flag 标记为 SUPPORTED。
 */
/* 初始化默认的DRAM信息 */
static void init_dram_info(void) {
  mutex_init(&g_curt_dram.lock);

  // 这里应该根据实际硬件信息初始化
  g_curt_dram.total_dram = 512;           // 假设总内存1GB
  g_curt_dram.os_managed_dram = 256;      // 默认OS管理512MB
  g_curt_dram.default_os_dram = 256;      // 默认OS内存512MB
  g_curt_dram.os_start_addr = 0x80000000; // 假设OS内存起始地址
  g_curt_dram.dyn_split_flag = DRAM_SPLIT_SUPPORTED; // 支持动态分割

  pr_info(PFX "Initialized: total_dram=%luMB, os_managed=%luMB\n",
          g_curt_dram.total_dram, g_curt_dram.os_managed_dram);
}

/**
 * do_dram_reallocate() - 对 OS 管理区的大小进行“重分配”（reallocate）
 * @total_os: 用户希望操作系统管理的内存总量，单位 MB
 *
 * 返回值：
 *   0        ：成功
 *  -EINVAL   ：参数不合法（如收缩场景暂不支持）
 *  -ENOMEM   ：请求超过可用物理内存
 *  -EPERM    ：当前分割状态不允许这样的操作
 *  其他负数 ：底层 memblock/free_bootmem 返回的错误码
 *
 * 主要步骤：
 *  1. 检查 dyn_split_flag：
 *     - 如果 NOT_SUPPORTED 且请求恰好等于 default_os，则正常返回 0；
 *     - 如果 NOT_SUPPORTED 且请求 != default_os，则返回 -EPERM；
 *     - 如果 ALREADY：表示已经做过一次 split，返回 -EPERM；
 *     - 如果 SUPPORTED：继续执行。
 *  2. 检查 total_os 的合法性：必须 ≤ total_dram；如果等于 os_managed_dram
 * 则直接返回 0；否则进入第 3 步。
 *  3. 如果 total_os > os_managed_dram（扩大 OS 管理区）：
 *       a) memblock_free() 释放保留区里从 old_os 到 total_dram 的整块；
 *       b) free_bootmem_late() 将新增部分页面交给 buddy 分配器；
 *       c) 如果在 HIGHMEM 区，则累加 totalhigh_pages；
 *       d) 更新 os_managed_dram = total_os；
 *  4. 如果 total_os < os_managed_dram（收缩 OS 管理区），当前示例不支持，返回
 * -EINVAL。
 *  5. memblock_remove() 从 memblock 中移除剩余的保留区（从新 os_managed 到
 * total_dram）。
 *  6. 设置 dyn_split_flag = ALREADY，返回 0。
 */
static int do_dram_reallocate(unsigned long total_os) {
  int ret = 0;

  mutex_lock(&g_curt_dram.lock);

  /* 1. 检查当前分割标志位 */
  switch (g_curt_dram.dyn_split_flag) {
  case DRAM_SPLIT_NOT_SUPPORTED:
    if (total_os == g_curt_dram.default_os_dram) {
      pr_info(
          PFX
          "设备共 %luMB，不支持动态分割，且请求大小 (%luMB) 恰好为默认值。\n",
          g_curt_dram.total_dram, total_os);
      mutex_unlock(&g_curt_dram.lock);
      return 0;
    } else {
      pr_err(PFX
             "设备共 %luMB，不支持动态分割！默认值：%luMB，应用请求：%luMB。\n",
             g_curt_dram.total_dram, g_curt_dram.default_os_dram, total_os);
      mutex_unlock(&g_curt_dram.lock);
      return -EPERM;
    }

  case DRAM_SPLIT_ALREADY:
    pr_err(PFX "DRAM 已经分割过！不允许重复操作。\n");
    mutex_unlock(&g_curt_dram.lock);
    return -EPERM;

  case DRAM_SPLIT_SUPPORTED:
    pr_info(PFX "准备将 DRAM 分割为 %luMB 给操作系统管理。\n", total_os);
    break;

  default:
    pr_err(PFX "无效的分割标志：%d\n", g_curt_dram.dyn_split_flag);
    mutex_unlock(&g_curt_dram.lock);
    return -EPERM;
  }

  /* 2. 检查请求合法性 */
  if (total_os > g_curt_dram.total_dram) {
    pr_err(PFX "请求 %luMB 超过总 DRAM %luMB！\n", total_os,
           g_curt_dram.total_dram);
    mutex_unlock(&g_curt_dram.lock);
    return -ENOMEM;
  }
  if (total_os == g_curt_dram.os_managed_dram) {
    pr_info(PFX "请求与当前 OS 管理区域一致 ( %luMB )，无需操作。\n",
            g_curt_dram.os_managed_dram);
    mutex_unlock(&g_curt_dram.lock);
    return 0;
  }

  /* 3. 如果需要扩大：total_os > 当前 os_managed_dram */
  if (total_os > g_curt_dram.os_managed_dram) {
    /* 假设你已经计算好了这些值 */
    phys_addr_t os_start = g_curt_dram.os_start_addr; // 0x80000000
    size_t old_os_size = 256 * SZ_1M;         // 256 * SZ_1M
    size_t new_os_size = 260 * SZ_1M;                 // 260 MiB
    size_t total_dram = 512 * SZ_1M;                  // 512 MiB

    /* 1. 先更新 g_curt_dram.os_size 为新的大小 */
    // g_curt_dram.os_size = new_os_size;

    /* 2. 计算真正的可释放区间 */
    phys_addr_t free_base = os_start + new_os_size;
    size_t free_sz = total_dram - new_os_size;

    /* 3. 从 memblock 中释放回可用内存区 */
    memblock_free(free_base, free_sz);

    /* 如果还有早期 bootmem，需要调用 free_bootmem_late() 也同理用
     * free_base/free_sz */
    free_bootmem_late(free_base, free_sz);

  }
  /* 4. 如果需要收缩：total_os < 当前 os_managed_dram */
  else if (total_os < g_curt_dram.os_managed_dram) {
    pr_err(PFX
           "当前版本暂不支持收缩操作 ( 从 %luMB 缩小到 %luMB )，请重新实现。\n",
           g_curt_dram.os_managed_dram, total_os);
    mutex_unlock(&g_curt_dram.lock);
    return -EINVAL;
  }

  /* 5. 从 memblock 中移除 “已释放但未分配给 buddy” 的剩余保留区 */
  {
    unsigned long remove_start =
        g_curt_dram.os_start_addr + (g_curt_dram.os_managed_dram << 20);
    unsigned long remove_size =
        (g_curt_dram.total_dram - g_curt_dram.os_managed_dram) << 20;
    // pr_info(PFX "memblock_remove：start=0x%lx, size=%luMB (移除保留区)\n",
    //         remove_start,
    //         (g_curt_dram.total_dram - g_curt_dram.os_managed_dram));
    pr_info(PFX "memblock_remove：start=0x%lx, remove_size=%luMB\n",
            remove_start, remove_size);
    memblock_remove(remove_start, remove_size);
  }

  /* 6. 更新标志 */
  g_curt_dram.dyn_split_flag = DRAM_SPLIT_ALREADY;
  pr_info(PFX "DRAM 分割完成：OS 管理区 = %luMB, 剩余保留区 = %luMB\n",
          g_curt_dram.os_managed_dram,
          (g_curt_dram.total_dram - g_curt_dram.os_managed_dram));

  mutex_unlock(&g_curt_dram.lock);
  return 0;
}

/* ====================== /proc 接口实现 ====================== */

/* proc 目录和文件名 */
#define PROC_DIR_NAME "dram_realloc_demo"
#define PROC_INFO_NAME "dram_info"
#define PROC_REALLOC_NAME "dram_realloc"

static struct proc_dir_entry *proc_dir;
static struct proc_dir_entry *proc_info_entry;
static struct proc_dir_entry *proc_realloc_entry;

/* helper：根据 flag 值返回字符串 */
static const char *flag_to_string(int flag) {
  switch (flag) {
  case DRAM_SPLIT_NOT_SUPPORTED:
    return "NOT_SUPPORTED";
  case DRAM_SPLIT_SUPPORTED:
    return "SUPPORTED";
  case DRAM_SPLIT_ALREADY:
    return "ALREADY";
  default:
    return "UNKNOWN";
  }
}

/* ==== 1. /proc/dram_info: 只读，输出当前 g_curt_dram 信息 ==== */

/* seq_file 的 show 函数，每次 read() 都会调用 */
static int dram_info_show(struct seq_file *m, void *v) {
  mutex_lock(&g_curt_dram.lock);
  seq_printf(m, "total_dram:       %lu MB\n", g_curt_dram.total_dram);
  seq_printf(m, "os_managed_dram:  %lu MB\n", g_curt_dram.os_managed_dram);
  seq_printf(m, "default_os_dram:  %lu MB\n", g_curt_dram.default_os_dram);
  seq_printf(m, "os_start_addr:    0x%lx\n", g_curt_dram.os_start_addr);
  seq_printf(m, "dyn_split_flag:   %s (%d)\n",
             flag_to_string(g_curt_dram.dyn_split_flag),
             g_curt_dram.dyn_split_flag);
  mutex_unlock(&g_curt_dram.lock);

  return 0;
}

/* open 函数：用于 seq_file 接口 */
static int dram_info_open(struct inode *inode, struct file *file) {
  return single_open(file, dram_info_show, NULL);
}

/* 4.1.15 中，proc_ops 已经可用 */
static struct file_operations dram_info_fops = {
    .owner = THIS_MODULE,
    .open = dram_info_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
};

/* ==== 2. /proc/dram_realloc: 写入新值 (MB) 触发 do_dram_reallocate ==== */

#define MAX_INPUT_LEN 32 /* 最多读取 32 字节，以防用户输入过长 */

static ssize_t dram_realloc_write(struct file *file, const char __user *buffer,
                                  size_t count, loff_t *ppos) {
  char *kbuf;
  unsigned long new_size_mb;
  int ret;

  if (count == 0)
    return 0;
  if (count > MAX_INPUT_LEN)
    return -EINVAL;

  kbuf = kzalloc(count + 1, GFP_KERNEL);
  if (!kbuf)
    return -ENOMEM;

  if (copy_from_user(kbuf, buffer, count)) {
    kfree(kbuf);
    return -EFAULT;
  }
  kbuf[count] = '\0'; /* 确保以 NULL 结尾 */

  /* 去掉末尾换行或空白，解析为整数 */
  sscanf(kbuf, "%lu", &new_size_mb);
  kfree(kbuf);

  pr_info(PFX "收到用户请求：reallocate OS 管理区到 %lu MB\n", new_size_mb);
  ret = do_dram_reallocate(new_size_mb);
  if (ret < 0) {
    pr_err(PFX "do_dram_reallocate(%lu) 失败，错误码：%d\n", new_size_mb, ret);
    return -EINVAL;
  }

  pr_info(PFX "do_dram_reallocate(%lu) 成功\n", new_size_mb);
  return count;
}

static struct file_operations dram_realloc_fops = {
    .owner = THIS_MODULE,
    .open = dram_info_open,
    .read = seq_read,
    .write = dram_realloc_write,
    .llseek = seq_lseek,
    .release = single_release,
};

/* ==== 3. 模块初始化/清理：创建和删除 proc 目录及文件 ==== */

static int __init dram_realloc_proc_init(void) {
  /* 先初始化全局 dram 信息 */
  init_dram_info();

  /* 在 /proc 下建立一个子目录 /proc/dram_realloc_demo */
  proc_dir = proc_mkdir(PROC_DIR_NAME, NULL);
  if (!proc_dir) {
    pr_err(PFX "无法创建 /proc/%s 目录\n", PROC_DIR_NAME);
    return -ENOMEM;
  }

  /* 创建 /proc/dram_realloc_demo/dram_info，只读 */
  proc_info_entry =
      proc_create(PROC_INFO_NAME, 0444, proc_dir, &dram_info_fops);
  if (!proc_info_entry) {
    pr_err(PFX "无法创建 /proc/%s/%s\n", PROC_DIR_NAME, PROC_INFO_NAME);
    remove_proc_entry(PROC_DIR_NAME, NULL);
    return -ENOMEM;
  }

  /* 创建 /proc/dram_realloc_demo/dram_realloc，只写 */
  proc_realloc_entry =
      proc_create(PROC_REALLOC_NAME, 0222, proc_dir, &dram_realloc_fops);
  if (!proc_realloc_entry) {
    pr_err(PFX "无法创建 /proc/%s/%s\n", PROC_DIR_NAME, PROC_REALLOC_NAME);
    remove_proc_entry(PROC_INFO_NAME, proc_dir);
    remove_proc_entry(PROC_DIR_NAME, NULL);
    return -ENOMEM;
  }

  pr_info(PFX "/proc 接口初始化完成：\n"
              "  - 阅读 DRAM 信息：  cat /proc/%s/%s\n"
              "  - 触发 reallocate： echo 260 > /proc/%s/%s\n",
          PROC_DIR_NAME, PROC_INFO_NAME, PROC_DIR_NAME, PROC_REALLOC_NAME);

  return 0;
}

static void __exit dram_realloc_proc_exit(void) {
  remove_proc_entry(PROC_REALLOC_NAME, proc_dir);
  remove_proc_entry(PROC_INFO_NAME, proc_dir);
  remove_proc_entry(PROC_DIR_NAME, NULL);
  pr_info(PFX "/proc 接口已移除\n");
}

module_init(dram_realloc_proc_init);
module_exit(dram_realloc_proc_exit);

// 	printk("[*********Function: %s, Line: %d ********]start:%ld end:%ld\n", __func__, __LINE__,start,end);

Hardware name: Freescale i.MX6 Ultralite (Device Tree)
[<80015c14>] (unwind_backtrace) from [<80012708>] (show_stack+0x10/0x14)
[<80012708>] (show_stack) from [<805ff3d8>] (dump_stack+0x80/0xc8)
[<805ff3d8>] (dump_stack) from [<800d4478>] (memblock_add_range+0x20/0x1f8)
[<800d4478>] (memblock_add_range) from [<800d4768>] (memblock_reserve_region.constprop.5+0x38/0x8c)
[<800d4768>] (memblock_reserve_region.constprop.5) from [<80857364>] (memblock_alloc_range_nid+0x38/0x4c)
[<80857364>] (memblock_alloc_range_nid) from [<80857558>] (__memblock_alloc_base+0x48/0x5c)
[<80857558>] (__memblock_alloc_base) from [<80857580>] (memblock_alloc_base+0x14/0x44)
[<80857580>] (memblock_alloc_base) from [<80868e98>] (early_init_dt_alloc_memory_arch+0xc/0x14)
[<80868e98>] (early_init_dt_alloc_memory_arch) from [<804f6778>] (of_alias_scan+0x17c/0x264)
[<804f6778>] (of_alias_scan) from [<80846020>] (setup_arch+0x704/0x98c)
[<80846020>] (setup_arch) from [<80842938>] (start_kernel+0x84/0x3a0)
[<80842938>] (start_kernel) from [<8000807c>] (0x8000807c)
