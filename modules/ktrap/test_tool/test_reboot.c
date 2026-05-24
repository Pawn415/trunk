#include <unistd.h>
#include <sys/reboot.h>
#include <linux/reboot.h>

int main(void)
{
    sync();

    reboot(RB_AUTOBOOT);

    return 0;
}