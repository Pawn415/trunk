#include "common.h"

void build_time_printf(void)
{
    printf("virt_App build time:%s %s\n", __DATE__, __TIME__);
}