#ifndef __COMMON_H_
#define __COMMON_H_

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <linux/ioctl.h>
#include <errno.h>

#define TYPE 'C'
#define CREATE_DEV_CMD _IOWR(TYPE, 0, int)
#define UNCREATE_DEV_CMD _IOWR(TYPE, 1, int)

#define CLOSE_CMD _IOWR(TYPE, 10, int)
#define OPEN_CMD _IOWR(TYPE, 11, int)
#define SETPERIOD_CMD _IOWR(TYPE, 12, int)

#define COMPLETION_CMD _IOWR(TYPE, 23, int)
#endif // !1
