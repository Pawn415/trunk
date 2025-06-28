// rcu_stall_module.c
#include <linux/module.h>
#include <linux/kthread.h>
#include <linux/rcupdate.h>
#include <linux/cpumask.h>
#include <linux/delay.h>

static struct task_struct *staller_tsk;

/* 这个线程会一直在 RCU 读侧临界区内循环，不调用 rcu_read_unlock() */
static int stall_fn(void *data)
{
    /* 绑定到 CPU0 */
    cpumask_t mask = CPU_MASK_NONE;
    cpumask_set_cpu(0, &mask);
    set_cpus_allowed_ptr(current, &mask);

    /* 等内核的 RCU grace-period kthread 跑起来，再开始忙等 */
    msleep(1000);

    /* 进入永不退出的 RCU 读侧临界区 */
    rcu_read_lock();
    pr_info("rcu_stall_module: entered RCU read-side critical section, looping...\n");
    while(1);
    // for (;;) {
    //     /* 保证 CPU 不做其它事 */
    //     cpu_relax();
    //     /* 永不 break 或调用 rcu_read_unlock() */
    // }
    /* 永远到不了这里 */
    rcu_read_unlock();
    return 0;
}

static int __init rcu_stall_init(void)
{
    pr_info("rcu_stall_module: loading, spawning staller thread\n");
    staller_tsk = kthread_run(stall_fn, NULL, "rcu_stall");
    if (IS_ERR(staller_tsk)) {
        pr_err("rcu_stall_module: failed to create thread\n");
        return PTR_ERR(staller_tsk);
    }
    return 0;
}

static void __exit rcu_stall_exit(void)
{
    if (staller_tsk)
        kthread_stop(staller_tsk);
    pr_info("rcu_stall_module: unloaded\n");
}

module_init(rcu_stall_init);
module_exit(rcu_stall_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Example");
MODULE_DESCRIPTION("Trigger rcu_preempt detected stalls on a single-CPU system");
