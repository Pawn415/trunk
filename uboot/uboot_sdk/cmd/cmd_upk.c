/*
 * Copyright 2000-2009
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

 #include <common.h>
 #include <command.h>
 #include <version.h>
 #include <linux/compiler.h>
 #ifdef CONFIG_SYS_COREBOOT
 #include <asm/arch/sysinfo.h>
 #endif
 
#ifndef DBG_PRINT_UPK
#define DBG_PRINT_UPK(fmt, ...) \
    printf("%s:%d: " fmt, __func__, __LINE__, ##__VA_ARGS__)
#endif
 
 static int do_upk(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
 {
    int ret;

    /* 参数校验 */
    if (argc != 1) {
        return CMD_RET_USAGE;
    }

    /* 1. TFTP 下载镜像到 0x80800000 */
    ret = run_command("tftp 0x80800000 uImage", flag);
    if (ret) {
        DBG_PRINT_UPK("Error: tftp failed (code=%d)\n", ret);
        return CMD_RET_FAILURE;
    }

    /* 2. 选择 mmc 设备 1, 分区 0 */
    ret = run_command("mmc dev 1 0", flag);
    if (ret) {
        DBG_PRINT_UPK("Error: mmc dev failed (code=%d)\n", ret);
        return CMD_RET_FAILURE;
    }

    /* 3. 从内存 0x80800000 写入到 eMMC 起始扇区 2，写入 1MiB (2048 扇区) */
    ret = run_command("mmc write 0x80800000 0x1000 0x8000", flag);
    if (ret) {
        DBG_PRINT_UPK("Error: mmc write failed (code=%d)\n", ret);
        return CMD_RET_FAILURE;
    }

    DBG_PRINT_UPK("upk ok\n");
    return CMD_RET_SUCCESS;
 }
 
 U_BOOT_CMD(
     upk,	1,		1,	do_upk,
     "update kernel to mmc",
     ""
 );
 