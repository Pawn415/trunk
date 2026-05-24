#include "ktrap.h"

static int die_event(struct notifier_block *nb,
                     unsigned long val,
                     void *data)
{
    struct die_args *args = data;

    save_log(
        "\n=========== DIE ===========\n"
        "reason  : %s\n"
        "process : %s\n"
        "pid     : %d\n"
        "pc      : %lx\n"
        "lr      : %lx\n"
        "===========================\n",
        args->str,
        current->comm,
        current->pid,
        instruction_pointer(args->regs),
        args->regs->ARM_lr);

    return NOTIFY_OK;
}

struct notifier_block die_nb = {
    .notifier_call = die_event,
};