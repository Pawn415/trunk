#include "ktrap.h"

static int oom_event(struct notifier_block *nb,
                     unsigned long val,
                     void *data)
{
    struct task_struct *p = data;

    if (!p)
        return NOTIFY_OK;

    save_log(
        "\n=========== OOM ===========\n"
        "victim      : %s\n"
        "pid         : %d\n"
        "current     : %s\n"
        "current pid : %d\n"
        "===========================\n",
        p->comm,
        p->pid,
        current->comm,
        current->pid);

    return NOTIFY_OK;
}

struct notifier_block oom_nb = {
    .notifier_call = oom_event,
};