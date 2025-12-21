#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/mutex.h>

// 默认配置
#define DEFAULT_REGISTER_ADDR 0X020C406C
#define DEFAULT_WRITE_VALUE 0xDEADBEEF
#define PROC_ENTRY_NAME "memw"

// 模块参数
static unsigned long register_addr = DEFAULT_REGISTER_ADDR;
static unsigned int write_value = DEFAULT_WRITE_VALUE;

// 全局变量
static void __iomem *target_reg = NULL;
static struct proc_dir_entry *proc_entry;
static DEFINE_MUTEX(reg_mutex);  // 保护寄存器操作

// 读取寄存器值
static u32 read_register(void)
{
    u32 value;
    
    if (!target_reg) {
        // 如果还没有映射，先映射
        target_reg = ioremap(register_addr, 4);
        if (!target_reg) {
            printk(KERN_ERR "Register Modifier: Failed to map register at 0x%08lx\n", register_addr);
            return 0;
        }
    }
    
    value = ioread32(target_reg);
    return value;
}

// 写入寄存器值
static int write_register(u32 value)
{
    if (!target_reg) {
        // 如果还没有映射，先映射
        target_reg = ioremap(register_addr, 4);
        if (!target_reg) {
            printk(KERN_ERR "Register Modifier: Failed to map register at 0x%08lx\n", register_addr);
            return -ENOMEM;
        }
    }
    
    iowrite32(value, target_reg);
    write_value = value;  // 更新模块参数
    printk(KERN_ERR "write virt 0x%08lx\n", target_reg);
    return 0;
}

// Proc文件读操作
static ssize_t reg_proc_read(struct file *file, char __user *buf, 
                            size_t count, loff_t *ppos)
{
    char info[256];
    u32 current_value;
    int len;
    
    if (*ppos > 0)
        return 0;
    
    mutex_lock(&reg_mutex);
    
    // 读取当前寄存器值
    current_value = read_register();
    
    // 格式化输出信息
    len = snprintf(info, sizeof(info),
                  "Register Modifier Status:\n"
                  "=======================\n"
                  "Physical Address: 0x%08lx\n"
                  "Mapped Virtual Address: %p\n"
                  "Current Register Value: 0x%08x\n"
                  "Default Write Value: 0x%08x\n\n"
                  "Usage:\n"
                  "echo 0x<value> > /proc/%s  # Write new value\n"
                  "cat /proc/%s               # Read current status\n",
                  register_addr, target_reg, current_value, 
                  write_value, PROC_ENTRY_NAME, PROC_ENTRY_NAME);
    
    mutex_unlock(&reg_mutex);
    
    if (copy_to_user(buf, info, len))
        return -EFAULT;
    
    *ppos = len;
    return len;
}

// Proc文件写操作
static ssize_t reg_proc_write(struct file *file, const char __user *buf,
                             size_t count, loff_t *ppos)
{
    char input[32];
    unsigned long new_value;
    int ret;
    
    if (count >= sizeof(input))
        return -EINVAL;
    
    if (copy_from_user(input, buf, count))
        return -EFAULT;
    
    input[count] = '\0';
    
    // 移除末尾的换行符
    if (input[count - 1] == '\n')
        input[count - 1] = '\0';
    
    // 解析输入的值（支持16进制和10进制）
    if (input[0] == '0' && (input[1] == 'x' || input[1] == 'X')) {
        // 16进制格式
        ret = kstrtoul(input, 16, &new_value);
    } else {
        // 10进制格式
        ret = kstrtoul(input, 10, &new_value);
    }
    
    if (ret) {
        printk(KERN_ERR "Register Modifier: Invalid value format: %s\n", input);
        return -EINVAL;
    }
    
    // 检查值是否在32位范围内
    if (new_value > 0xFFFFFFFFUL) {
        printk(KERN_ERR "Register Modifier: Value too large: 0x%lx\n", new_value);
        return -EINVAL;
    }
    
    mutex_lock(&reg_mutex);
    
    // 写入新值
    ret = write_register((u32)new_value);
    if (ret) {
        mutex_unlock(&reg_mutex);
        return ret;
    }
    
    // 验证写入
    u32 verify_value = read_register();
    
    mutex_unlock(&reg_mutex);
    
    printk(KERN_INFO "Register Modifier: Wrote 0x%08x to register 0x%08lx, read back 0x%08x\n",
           (u32)new_value, register_addr, verify_value);
    
    return count;
}

// 文件操作结构体
static const struct file_operations reg_proc_fops = {
    .owner = THIS_MODULE,
    .read = reg_proc_read,
    .write = reg_proc_write,
};

// 初始化函数
static int __init register_modifier_init(void)
{
    printk(KERN_INFO "Register Modifier: Loading\n");
    
    // 创建proc入口
    proc_entry = proc_create(PROC_ENTRY_NAME, 0644, NULL, &reg_proc_fops);
    if (!proc_entry) {
        printk(KERN_ERR "Register Modifier: Failed to create proc entry\n");
        return -ENOMEM;
    }
    
    // 初始映射和读取
    mutex_lock(&reg_mutex);
    target_reg = ioremap(register_addr, 4);
    if (!target_reg) {
        mutex_unlock(&reg_mutex);
        proc_remove(proc_entry);
        printk(KERN_ERR "Register Modifier: Failed to map register at 0x%08lx\n", register_addr);
        return -ENOMEM;
    }
    
    u32 original_value = ioread32(target_reg);
    mutex_unlock(&reg_mutex);
    
    printk(KERN_INFO "Register Modifier: Mapped virtual address = %p\n", target_reg);
    printk(KERN_INFO "Register Modifier: Register 0x%08lx original value = 0x%08x\n", 
           register_addr, original_value);
    
    printk(KERN_INFO "Register Modifier: Loaded successfully\n");
    printk(KERN_INFO "Register Modifier: Proc interface at /proc/%s\n", PROC_ENTRY_NAME);
    
    return 0;
}

// 清理函数
static void __exit register_modifier_exit(void)
{
    // 移除proc入口
    if (proc_entry)
        proc_remove(proc_entry);
    
    // 取消映射
    if (target_reg)
        iounmap(target_reg);
    
    printk(KERN_INFO "Register Modifier: Unloaded\n");
}

module_init(register_modifier_init);
module_exit(register_modifier_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Register Modifier with Proc Interface");