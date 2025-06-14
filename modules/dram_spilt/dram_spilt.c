/* dram_realloc_driver.c
 *
 * A simple Linux kernel module to adjust the OS-managed DRAM region via a /proc
 * interface. Writing a new size (in MB) to /proc/dram_realloc will reallocate
 * the remaining memory back to the buddy allocator. Reading from
 * /proc/dram_realloc prints the current OS region base and size.
 *
 * Assumptions:
 *   - Physical DRAM starts at DRAM_BASE and has TOTAL_DRAM_MB size.
 *   - g_curt_dram.os_start_addr and g_curt_dram.os_size track current OS
 * region.
 *   - memblock and free_bootmem_late are available.
 */

#include <linux/init.h>
#include <linux/memblock.h>
#include <linux/bootmem.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>

#define SIZE_1M 0x100000UL
#define PROC_NAME "dram_realloc"
#define DRAM_BASE 0x80000000UL /* Example base address */
#define TOTAL_DRAM_MB 512      /* Total DRAM size in MB */

extern int memblock_free(phys_addr_t base, phys_addr_t size);
extern int memblock_remove(phys_addr_t base, phys_addr_t size);

struct {
  phys_addr_t os_start_addr;
  size_t os_size; /* in bytes */
} g_curt_dram;

static int dram_realloc_show(struct seq_file *m, void *v) {
  seq_printf(m, "OS region start: 0x%llx, size: %zu MB\n",
             (unsigned long long)g_curt_dram.os_start_addr,
             g_curt_dram.os_size >> 20);
  return 0;
}

static ssize_t dram_realloc_write(struct file *file, const char __user *buffer,
                                  size_t count, loff_t *ppos) {
  char buf[32];
  unsigned long new_mb;
  phys_addr_t new_end;
  size_t free_sz;

  if (count >= sizeof(buf))
    return -EINVAL;
  if (copy_from_user(buf, buffer, count))
    return -EFAULT;
  buf[count] = '\0';

  if (kstrtoul(buf, 10, &new_mb))
    return -EINVAL;

  if (new_mb > TOTAL_DRAM_MB)
    return -EINVAL;

  pr_info("dram_realloc: freeing:%d MB\n", new_mb);
  /* Compute free region: from new_end to physical DRAM end */
  new_end = g_curt_dram.os_start_addr + 256 *SIZE_1M;
  free_sz = new_mb *SIZE_1M;

  /* Return region to memblock and bootmem */
  pr_info("dram_realloc: freeing [0x%lx - +%lx bytes]\n",(unsigned long)new_end, free_sz);
  memblock_free(new_end, 256 *SIZE_1M);
  free_bootmem_late(new_end, free_sz);

  return count;
}

static int dram_realloc_open(struct inode *inode, struct file *file) {
  return single_open(file, dram_realloc_show, NULL);
}

static const struct file_operations proc_fops = {
    .owner = THIS_MODULE,
    .open = dram_realloc_open,
    .read = seq_read,
    .write = dram_realloc_write,
    .llseek = seq_lseek,
    .release = single_release,
};

static int __init dram_realloc_init(void) {
  /* Initialize with default OS region: full DRAM */
  g_curt_dram.os_start_addr = DRAM_BASE;
  g_curt_dram.os_size = TOTAL_DRAM_MB << 20;

  proc_create(PROC_NAME, 0666, NULL, &proc_fops);
  pr_info("dram_realloc module loaded. Use /proc/%s to adjust OS DRAM size.\n",
          PROC_NAME);
  return 0;
}

static void __exit dram_realloc_exit(void) {
  remove_proc_entry(PROC_NAME, NULL);
  pr_info("dram_realloc module unloaded.\n");
}

module_init(dram_realloc_init);
module_exit(dram_realloc_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("DRAM reallocation via proc interface");
MODULE_VERSION("1.0");


// 262144-234136=28008