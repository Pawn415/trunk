#include "ktrap.h"

char *log_buf;
int log_len;

static DEFINE_SPINLOCK(log_lock);

/************************************************
 * 保存日志
 ************************************************/
void save_log(const char *fmt, ...)
{
    va_list args;
    unsigned long flags;
    int remain;
    int len;

    if (!log_buf)
        return;

    spin_lock_irqsave(&log_lock, flags);

    remain = LOG_BUF_SIZE - log_len;

    if (remain <= 0) {
        spin_unlock_irqrestore(&log_lock, flags);
        return;
    }

    va_start(args, fmt);

    len = vscnprintf(log_buf + log_len,
                     remain,
                     fmt,
                     args);

    va_end(args);

    log_len += len;

    spin_unlock_irqrestore(&log_lock, flags);
}

/************************************************
 * proc read
 ************************************************/
static ssize_t proc_read(struct file *file,
                         char __user *buf,
                         size_t count,
                         loff_t *ppos)
{
    return simple_read_from_buffer(buf,
                                   count,
                                   ppos,
                                   log_buf,
                                   log_len);
}

static const struct file_operations proc_fops = {
    .owner = THIS_MODULE,
    .read  = proc_read,
};

/************************************************
 * init
 ************************************************/
static int __init ktrap_init(void)
{
    printk(KERN_INFO "ktrap init\n");

    log_buf = kzalloc(LOG_BUF_SIZE, GFP_KERNEL);
    if (!log_buf)
        return -ENOMEM;

    proc_create("ktrap_info",
                0444,
                NULL,
                &proc_fops);

    register_reboot_notifier(&reboot_nb);

    atomic_notifier_chain_register(
            &panic_notifier_list,
            &panic_nb);

    register_die_notifier(&die_nb);

    atomic_notifier_chain_register(
            &my_oom_chain,
            &oom_nb);

    return 0;
}

/************************************************
 * exit
 ************************************************/
static void __exit ktrap_exit(void)
{
    unregister_reboot_notifier(&reboot_nb);

    atomic_notifier_chain_unregister(
            &panic_notifier_list,
            &panic_nb);

    unregister_die_notifier(&die_nb);

    atomic_notifier_chain_unregister(
            &my_oom_chain,
            &oom_nb);

    remove_proc_entry("ktrap_info", NULL);

    kfree(log_buf);
}

module_init(ktrap_init);
module_exit(ktrap_exit);

MODULE_LICENSE("GPL");