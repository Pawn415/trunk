#ifndef _KTRAP_H
#define _KTRAP_H

#include <linux/kernel.h>
#include <linux/notifier.h>
#include <linux/proc_fs.h>
#include <linux/reboot.h>
#include <linux/uaccess.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/kdebug.h>
#include <linux/module.h>

#define LOG_BUF_SIZE   4096

extern char *log_buf;
extern int log_len;

extern struct atomic_notifier_head my_oom_chain;

/*
 * 公共函数
 */
void save_log(const char *fmt, ...);

/*
 * notifier block
 */
extern struct notifier_block reboot_nb;
extern struct notifier_block panic_nb;
extern struct notifier_block die_nb;
extern struct notifier_block oom_nb;

#endif