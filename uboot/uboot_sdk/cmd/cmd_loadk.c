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
 
 #ifndef DBG_PRINT_LOADK
 #define DBG_PRINT_LOADK(fmt, ...) \
     printf("%s:%d: " fmt, __func__, __LINE__, ##__VA_ARGS__)
 #endif
 
 static int do_loadk(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
 {
    int ret;

    /* 参数校验 */
    if (argc != 1) {
        return CMD_RET_USAGE;
    }

    /* 2. 选择 mmc 设备 1, 分区 0 */
    ret = run_command("mmc dev 1 0", flag);
    if (ret) {
        DBG_PRINT_LOADK("Error: mmc dev failed (code=%d)\n", ret);
        return CMD_RET_FAILURE;
    }

    /* 3. 从内存 0x80800000 写入到 eMMC 起始扇区 2，写入 1MiB (2048 扇区) */
    ret = run_command("mmc read 0x80800000 2050 30720", flag);
    if (ret) {
        DBG_PRINT_LOADK("Error: mmc write failed (code=%d)\n", ret);
        return CMD_RET_FAILURE;
    }

    DBG_PRINT_LOADK("loadk ok\n");
    return CMD_RET_SUCCESS;
 }
 
 U_BOOT_CMD(
     loadk,	1,		1,	do_loadk,
     "load uImage to ddr",
     ""
 );
 