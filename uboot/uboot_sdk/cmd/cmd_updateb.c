#include <common.h>
#include <command.h>

/* 调试打印宏，自动输出函数名和行号 */
#ifndef DBG_PRINT
#define DBG_PRINT(fmt, ...) \
    printf("%s:%d: " fmt, __func__, __LINE__, ##__VA_ARGS__)
#endif

/* 自定义命令处理函数 */
static int do_updateb(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
    int ret;

    /* 参数校验 */
    if (argc != 1) {
        return CMD_RET_USAGE;
    }

    /* 1. TFTP 下载镜像到 0x80800000 */
    ret = run_command("tftp 0x80800000 u-boot.imx", flag);
    if (ret) {
        DBG_PRINT("Error: tftp failed (code=%d)\n", ret);
        return CMD_RET_FAILURE;
    }

    /* 2. 选择 mmc 设备 1, 分区 0 */
    ret = run_command("mmc dev 1 0", flag);
    if (ret) {
        DBG_PRINT("Error: mmc dev failed (code=%d)\n", ret);
        return CMD_RET_FAILURE;
    }

    /* 3. 从内存 0x80800000 写入到 eMMC 起始扇区 2，写入 1MiB (2048 扇区) */
    ret = run_command("mmc write 0x80800000 2 0x7FE", flag);
    if (ret) {
        DBG_PRINT("Error: mmc write failed (code=%d)\n", ret);
        return CMD_RET_FAILURE;
    }

    /* 4. 设置分区 1 为可启动 (bootable=1)，readonly=0，hidden=0 */
    ret = run_command("mmc partconf 1 1 0 0", flag);
    if (ret) {
        DBG_PRINT("Error: mmc partconf failed (code=%d)\n", ret);
        return CMD_RET_FAILURE;
    }

    DBG_PRINT("updateb ok\n");
    ret = run_command("re", flag);
    if (ret) {
        DBG_PRINT("Error: mmc partconf failed (code=%d)\n", ret);
        return CMD_RET_FAILURE;
    }
    return CMD_RET_SUCCESS;
}

/* 把命令注册到 U-Boot */
U_BOOT_CMD(
    updateb,               /* 命令名 */
    1,                     /* 最大参数数目 */
    0,                     /* 直接重入(1)或不可重入(0) */
    do_updateb,            /* 处理函数 */
    "update uboot image", /* 简短描述 */
    "updateb test" /* 用法说明 */
);
