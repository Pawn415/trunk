#include "ktrap.h"

static int reboot_event(struct notifier_block *nb,
                        unsigned long action,
                        void *data)
{
    save_log(
        "\n========== REBOOT ==========\n"
        "process : %s\n"
        "pid     : %d\n"
        "action  : %lu\n"
        "cmd     : %s\n"
        "============================\n",
        current->comm,
        current->pid,
        action,
        data ? (char *)data : "none");

        printk(
            "\n========== REBOOT ==========\n"
            "process : %s\n"
            "pid     : %d\n"
            "action  : %lu\n"
            "cmd     : %s\n"
            "============================\n",
            current->comm,
            current->pid,
            action,
            data ? (char *)data : "none");

    return NOTIFY_OK;
}

struct notifier_block reboot_nb = {
    .notifier_call = reboot_event,
};