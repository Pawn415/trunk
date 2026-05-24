#include "ktrap.h"

extern struct atomic_notifier_head panic_notifier_list;

static int panic_event(struct notifier_block *nb,
                       unsigned long event,
                       void *ptr)
{
    save_log(
        "\n========== PANIC ==========\n"
        "process : %s\n"
        "pid     : %d\n"
        "===========================\n",
        current->comm,
        current->pid);

    return NOTIFY_OK;
}

struct notifier_block panic_nb = {
    .notifier_call = panic_event,
};