#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/version.h>
extern unsigned char debug_uart_buff[1024];
extern unsigned char debug_uart_cnt;
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 6, 0)
// 对于 5.6 以下的内核，使用 file_operations 而不是 proc_ops
#include <linux/fs.h>
#endif

static struct task_struct *my_kthread = NULL;
static struct proc_dir_entry *my_proc_file = NULL;
static int thread_running = 0;  // 0: 停止，1: 运行

// 线程函数
static int my_thread_func(void *data)
{
    int i = 0;
    
    while (!kthread_should_stop()) {

		if (strstr(debug_uart_buff, "hql") != NULL) {

			printk("debug_uart_buff:");
			for (i=0; i<debug_uart_cnt; i++) {
				printk("%c", debug_uart_buff[i]);}
				debug_uart_cnt = 0;
				memset(debug_uart_buff, 0, 1024);
			}

			

    }
    return 0;
}

// proc 文件写操作
static ssize_t my_proc_write(struct file *file, const char __user *buffer,
                             size_t count, loff_t *pos)
{
    char cmd;
    int ret = count;
    
    if (count < 1)
        return -EINVAL;
    
    if (copy_from_user(&cmd, buffer, 1))
        return -EFAULT;
    
    if (cmd == '1') {
        if (!thread_running) {
            my_kthread = kthread_run(my_thread_func, NULL, "my_kthread");
            if (IS_ERR(my_kthread)) {
                printk(KERN_ERR "Failed to create thread, error: %ld\n", PTR_ERR(my_kthread));
                ret = PTR_ERR(my_kthread);
            } else {
                thread_running = 1;
                printk(KERN_INFO "Thread started\n");
            }
        } else {
            printk(KERN_INFO "Thread already running\n");
        }
    } else if (cmd == '0') {
        // 添加停止功能
        if (thread_running && my_kthread) {
            kthread_stop(my_kthread);
            thread_running = 0;
            my_kthread = NULL;
            printk(KERN_INFO "Thread stopped\n");
        } else {
            printk(KERN_INFO "Thread not running\n");
        }
    } else {
        printk(KERN_INFO "Invalid command. Use:\n");
        printk(KERN_INFO "  echo 1 > /proc/debug_uart  # 启动线程\n");
        printk(KERN_INFO "  echo 0 > /proc/debug_uart  # 停止线程\n");
        ret = -EINVAL;
    }
    
    return ret;
}

// proc 文件读操作（可选，用于查看状态）
static ssize_t my_proc_read(struct file *file, char __user *buffer,
                           size_t count, loff_t *pos)
{
    char status_msg[128];
    int len;
    
    if (*pos > 0)
        return 0;
    
    len = snprintf(status_msg, sizeof(status_msg),
                  "Thread status: %s\nCommands:\n  1 - Start thread\n  0 - Stop thread\n",
                  thread_running ? "RUNNING" : "STOPPED");
    
    if (len > count)
        len = count;
    
    if (copy_to_user(buffer, status_msg, len))
        return -EFAULT;
    
    *pos = len;
    return len;
}

// 4.1.15 内核使用 file_operations 结构
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 6, 0)
static const struct file_operations my_proc_fops = {
    .owner = THIS_MODULE,
    .read = my_proc_read,
    .write = my_proc_write,
};
#else
// 5.6+ 内核使用 proc_ops 结构
static const struct proc_ops my_proc_fops = {
    .proc_read = my_proc_read,
    .proc_write = my_proc_write,
};
#endif

static int __init my_module_init(void)
{
    // printk(KERN_INFO "Module init, kernel version: %s\n", UTS_RELEASE);
    
    // 创建 proc 文件
    my_proc_file = proc_create("debug_uart", 0644, NULL, &my_proc_fops);
    if (!my_proc_file) {
        printk(KERN_ERR "Failed to create proc file\n");
        return -ENOMEM;
    }
    
    printk(KERN_INFO "Proc interface created at /proc/debug_uart\n");
    printk(KERN_INFO "Use: echo 1 > /proc/debug_uart to start thread\n");
    
    return 0;
}

static void __exit my_module_exit(void)
{
    printk(KERN_INFO "Module exit\n");
    
    // 停止线程
    if (thread_running && my_kthread) {
        kthread_stop(my_kthread);
        thread_running = 0;
    }
    
    // 删除 proc 文件
    if (my_proc_file) {
        remove_proc_entry("debug_uart", NULL);
        my_proc_file = NULL;
    }
    
    printk(KERN_INFO "Module unloaded\n");
}

module_init(my_module_init);
module_exit(my_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Kernel thread with proc control for 4.1.15");
MODULE_VERSION("1.0");