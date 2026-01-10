/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License v2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 021110-1307, USA.
 *
 * Modified to add field firmware update support,
 * those modifications are Copyright (c) 2016 SanDisk Corp.
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <sys/ioctl.h>
 #include <sys/types.h>
 #include <sys/stat.h>
 #include <unistd.h>
 #include <fcntl.h>
 #include <errno.h>
 #include <stdint.h>
 #include <assert.h>
 #include <linux/fs.h> /* for BLKGETSIZE */

#include <sys/socket.h>
#include <signal.h>
#include <linux/netlink.h>
#include <endian.h>  // 用于字节序转换

 #include "mmc.h"
 #include "mmc_cmds.h"
 #include "3rdparty/hmac_sha/hmac_sha2.h"
 
 #ifndef offsetof
 #define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
 #endif
 
 #define WP_BLKS_PER_QUERY 32
 
 #define USER_WP_PERM_PSWD_DIS	0x80
 #define USER_WP_CD_PERM_WP_DIS	0x40
 #define USER_WP_US_PERM_WP_DIS	0x10
 #define USER_WP_US_PWR_WP_DIS	0x08
 #define USER_WP_US_PERM_WP_EN	0x04
 #define USER_WP_US_PWR_WP_EN	0x01
 #define USER_WP_CLEAR (USER_WP_US_PERM_WP_DIS | USER_WP_US_PWR_WP_DIS	\
			 | USER_WP_US_PERM_WP_EN | USER_WP_US_PWR_WP_EN)
 
 #define WPTYPE_NONE 0
 #define WPTYPE_TEMP 1
 #define WPTYPE_PWRON 2
 #define WPTYPE_PERM 3
 #define MMC_RSP_SPI_S1	(1 << 7)		/* one status byte */
#define MMC_RSP_SPI_S2	(1 << 8)		/* second byte */

 #define MMC_RSP_SPI_R2	(MMC_RSP_SPI_S1|MMC_RSP_SPI_S2)
 /* Extended CSD寄存器索引定义 */
 #define EXT_CSD_DEVICE_TYPE         196
 #define EXT_CSD_EXT_CSD_REV         192
 #define EXT_CSD_PARTITIONING_SUPPORT 160
 #define EXT_CSD_PARTITIONS_ATTRIBUTE 156
 #define EXT_CSD_ENH_START_ADDR      136
 #define EXT_CSD_ENH_SIZE_MULT       140
 #define EXT_CSD_GP_SIZE_MULT        143
 #define EXT_CSD_WR_REL_PARAM        166
 #define EXT_CSD_WR_REL_SET          167
 #define EXT_CSD_RPMB_SIZE_MULT      168
 #define EXT_CSD_FW_CONFIG           169
 #define EXT_CSD_BOOT_SIZE_MULT      226
 #define EXT_CSD_HPI_MGMT            251
 #define EXT_CSD_S_A_TIMEOUT         217
 
 /* 通用错误码 */
 #define EMMC_SUCCESS                0
 #define EMMC_ERROR_IOCTL            -1
 #define EMMC_ERROR_INVALID_PARAM    -2
 #define EMMC_ERROR_CMD_FAILED       -3
 
 /* 扩展CSD命令集 */
 #define EXT_CSD_CMD_SET_SECURE      1
 #define EXT_CSD_CMD_SET_CPSECURE    2
 
 /* MMC_SWITCH参数构造 */
 #define MMC_SWITCH_MODE_WRITE_BYTE  0x03
 #define MMC_SWITCH_MODE_WRITE_WORD  0x04
 #define MMC_SWITCH_MODE_CLEAR_BITS  0x02
 #define MMC_SWITCH_MODE_SET_BITS    0x01
 
 /* 检查内核版本是否支持error字段 */
 #ifdef MMC_IOC_CMD_HAS_ERROR
 /* 新版本内核有error字段 */
 #define CHECK_MMC_ERROR(cmd) ((cmd).error)
 #else
 /* 旧版本内核没有error字段 */
 #define CHECK_MMC_ERROR(cmd) (0)
 #endif
 
 struct mmc_netlink_stats  {
    unsigned long read_count;          // 读操作次数
    unsigned long write_count;         // 写操作次数
    unsigned long long total_write_bytes;  // 累计读取量，以MB为单位
    unsigned long long total_read_bytes; // 累计写入量，以MB为单位
	unsigned long long total_write_MB;  // 累计读取量，以MB为单位
    unsigned long long total_read_MB; // 累计写入量，以MB为单位
    
	char msg[128];
};

#define NETLINK_USER 31  // 自定义协议号
#define MAX_MSG_SIZE 1024

static int nl_fd = -1;
static int running = 1;

// 信号处理
static void handle_signal(int sig) {
    printf("收到信号 %d，退出程序\n", sig);
    running = 0;
}

// 初始化 Netlink 套接字
static int init_netlink(void) {
    struct sockaddr_nl addr;
    
    // 创建套接字
    nl_fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_USER);
    if (nl_fd < 0) {
        perror("socket");
        return -1;
    }
    
    // 设置地址并绑定
    memset(&addr, 0, sizeof(addr));
    addr.nl_family = AF_NETLINK;
    addr.nl_pid = getpid();  // 进程ID作为地址
    addr.nl_groups = 0;
    
    if (bind(nl_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(nl_fd);
        return -1;
    }
    
    return 0;
}

// 发送消息到内核
static void send_msg(const char *msg) {
    struct sockaddr_nl dest_addr;
    struct nlmsghdr *nlh;
    struct iovec iov;
    struct msghdr msg_hdr;
    
    // 分配消息缓冲区
    nlh = (struct nlmsghdr*)malloc(NLMSG_SPACE(MAX_MSG_SIZE));
    memset(nlh, 0, NLMSG_SPACE(MAX_MSG_SIZE));
    
    // 填充消息头
    nlh->nlmsg_len = NLMSG_SPACE(MAX_MSG_SIZE);
    nlh->nlmsg_pid = getpid();
    nlh->nlmsg_flags = 0;
    
    // 填充消息内容
    strncpy(NLMSG_DATA(nlh), msg, MAX_MSG_SIZE-1);
    
    // 设置目的地址（内核）
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.nl_family = AF_NETLINK;
    dest_addr.nl_pid = 0;      // 0 表示发送到内核
    dest_addr.nl_groups = 0;
    
    // 发送消息
    iov.iov_base = (void*)nlh;
    iov.iov_len = nlh->nlmsg_len;
    
    msg_hdr.msg_name = (void*)&dest_addr;
    msg_hdr.msg_namelen = sizeof(dest_addr);
    msg_hdr.msg_iov = &iov;
    msg_hdr.msg_iovlen = 1;
    
    sendmsg(nl_fd, &msg_hdr, 0);
    printf("发送消息: %s\n", msg);
    
    free(nlh);
}


static void convert_stats_to_host(struct mmc_netlink_stats *stats)
{
    // 对于64位整型
    stats->total_write_bytes = be64toh(stats->total_write_bytes);
    stats->total_write_bytes = be64toh(stats->total_write_bytes);
    
    // 对于32位整型（假设unsigned long是32位）
    stats->read_count = be32toh(stats->read_count);
    stats->write_count = be32toh(stats->write_count);
}


// 接收消息
static void recv_msg(void) {
    char buffer[2048];
    struct sockaddr_nl src_addr;
    struct nlmsghdr *nlh;
    struct msghdr msg;
    struct iovec iov;
    int len;
    
    iov.iov_base = buffer;
    iov.iov_len = sizeof(buffer);
    
    memset(&msg, 0, sizeof(msg));
    msg.msg_name = &src_addr;
    msg.msg_namelen = sizeof(src_addr);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    
    // 非阻塞接收
    len = recvmsg(nl_fd, &msg, MSG_DONTWAIT);
    if (len <= 0) return;
    
    // 处理接收到的每个消息
    for (nlh = (struct nlmsghdr*)buffer; NLMSG_OK(nlh, len); 
         nlh = NLMSG_NEXT(nlh, len)) {
        if (nlh->nlmsg_type == NLMSG_ERROR) {
            printf("收到错误消息\n");
        } else if (nlh->nlmsg_type == NLMSG_DONE) {
            // 从netlink消息中提取结构体数据
            struct mmc_netlink_stats *stats = (struct mmc_netlink_stats *)NLMSG_DATA(nlh);
            
            // 字节序转换（如果需要）
            // convert_stats_to_host(stats);
            
            // 打印结构体数据
            printf("\n========== 收到MMC统计信息 ==========\n");
            printf("消息标题: %s\n", stats->msg);
            printf("读操作次数: %lu\n", stats->read_count);
            printf("写操作次数: %lu\n", stats->write_count);
			printf("累计读取量: %llu Mb\n", stats->total_read_MB);
            printf("累计写入量: %llu Mb\n", stats->total_write_MB);
            
            printf("=======================================\n");
            
            // 可选：将数据保存到文件
            // save_stats_to_file(stats);
        } else {
            printf("收到未知类型的消息: %d\n", nlh->nlmsg_type);
        }
    }
}

// 守护进程化
static void become_daemon(void) {
    if (fork() > 0) exit(0);  // 父进程退出
    setsid();                  // 创建新会话
}


 /**
  * @brief 读取扩展CSD寄存器
  * 
  * @param fd 设备文件描述符
  * @param ext_csd 扩展CSD数据缓冲区（必须至少512字节）
  * @return int 成功返回0，失败返回错误码
  */
 int read_extcsd(int fd, __u8 *ext_csd)
 {
	 int ret = 0;
	 struct mmc_ioc_cmd idata;
	 
	 if (fd < 0) {
		 fprintf(stderr, "Invalid file descriptor\n");
		 return EMMC_ERROR_INVALID_PARAM;
	 }
	 
	 if (!ext_csd) {
		 fprintf(stderr, "ext_csd buffer is NULL\n");
		 return EMMC_ERROR_INVALID_PARAM;
	 }
	 
	 memset(&idata, 0, sizeof(idata));
	 memset(ext_csd, 0, sizeof(__u8) * 512);
	 
	 idata.write_flag = 0;
	 idata.opcode = MMC_SEND_EXT_CSD;
	 idata.arg = 0;
	 idata.flags = MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_ADTC;
	 idata.blksz = 512;
	 idata.blocks = 1;
	 mmc_ioc_cmd_set_data(idata, ext_csd);
 
	 ret = ioctl(fd, MMC_IOC_CMD, &idata);
	 if (ret) {
		 perror("ioctl MMC_SEND_EXT_CSD failed");
		 return EMMC_ERROR_IOCTL;
	 }
	 
	 /* 检查命令执行状态 - 兼容不同内核版本 */
	 if (CHECK_MMC_ERROR(idata)) {
		 fprintf(stderr, "MMC command error: %d\n", CHECK_MMC_ERROR(idata));
		 return EMMC_ERROR_CMD_FAILED;
	 }
 
	 return EMMC_SUCCESS;
 }
 
 /**
  * @brief 写入扩展CSD寄存器单个字节
  * 
  * @param fd 设备文件描述符
  * @param index 扩展CSD寄存器索引
  * @param value 要写入的值
  * @return int 成功返回0，失败返回错误码
  */
 int write_extcsd_value(int fd, __u8 index, __u8 value)
 {
	 int ret = 0;
	 struct mmc_ioc_cmd idata;
 
	 if (fd < 0) {
		 fprintf(stderr, "Invalid file descriptor\n");
		 return EMMC_ERROR_INVALID_PARAM;
	 }
 
	 memset(&idata, 0, sizeof(idata));
	 idata.write_flag = 1;
	 idata.opcode = MMC_SWITCH;
	 idata.arg = (MMC_SWITCH_MODE_WRITE_BYTE << 24) |
			 (index << 16) |
			 (value << 8) |
			 EXT_CSD_CMD_SET_NORMAL;
	 idata.flags = MMC_RSP_SPI_R1B | MMC_RSP_R1B | MMC_CMD_AC;
 
	 ret = ioctl(fd, MMC_IOC_CMD, &idata);
	 if (ret) {
		 perror("ioctl MMC_SWITCH failed");
		 return EMMC_ERROR_IOCTL;
	 }
	 
	 /* 检查命令执行状态 - 兼容不同内核版本 */
	 if (CHECK_MMC_ERROR(idata)) {
		 fprintf(stderr, "MMC command error: %d\n", CHECK_MMC_ERROR(idata));
		 return EMMC_ERROR_CMD_FAILED;
	 }
 
	 return EMMC_SUCCESS;
 }
 
 /**
  * @brief 批量写入扩展CSD寄存器
  * 
  * @param fd 设备文件描述符
  * @param index 起始寄存器索引
  * @param data 数据缓冲区
  * @param len 数据长度（字节数）
  * @return int 成功返回0，失败返回错误码
  */
 int write_extcsd_block(int fd, __u8 start_index, const __u8 *data, size_t len)
 {
	 int ret = 0;
	 struct mmc_ioc_cmd idata;
	 __u8 *buffer;
	 
	 if (fd < 0 || !data || len == 0) {
		 fprintf(stderr, "Invalid parameters\n");
		 return EMMC_ERROR_INVALID_PARAM;
	 }
	 
	 if (len > 512) {
		 fprintf(stderr, "Data length too large (max 512 bytes)\n");
		 return EMMC_ERROR_INVALID_PARAM;
	 }
	 
	 /* 分配缓冲区并填充数据 */
	 buffer = malloc(len);
	 if (!buffer) {
		 perror("Failed to allocate buffer");
		 return -ENOMEM;
	 }
	 
	 memcpy(buffer, data, len);
	 
	 memset(&idata, 0, sizeof(idata));
	 idata.write_flag = 1;
	 idata.opcode = MMC_SWITCH;
	 idata.arg = (MMC_SWITCH_MODE_WRITE_BYTE << 24) |
			 (start_index << 16) |
			 (len << 8) |
			 EXT_CSD_CMD_SET_NORMAL;
	 idata.flags = MMC_RSP_SPI_R1B | MMC_RSP_R1B | MMC_CMD_ADTC;
	 idata.blksz = len;
	 idata.blocks = 1;
	 mmc_ioc_cmd_set_data(idata, buffer);
 
	 ret = ioctl(fd, MMC_IOC_CMD, &idata);
	 if (ret) {
		 perror("ioctl MMC_SWITCH block failed");
		 free(buffer);
		 return EMMC_ERROR_IOCTL;
	 }
	 
	 /* 检查命令执行状态 - 兼容不同内核版本 */
	 if (CHECK_MMC_ERROR(idata)) {
		 fprintf(stderr, "MMC command error: %d\n", CHECK_MMC_ERROR(idata));
		 free(buffer);
		 return EMMC_ERROR_CMD_FAILED;
	 }
	 
	 free(buffer);
	 return EMMC_SUCCESS;
 }
 
 /**
  * @brief 读取单个扩展CSD寄存器值
  * 
  * @param fd 设备文件描述符
  * @param index 寄存器索引
  * @param value 返回的值
  * @return int 成功返回0，失败返回错误码
  */
 int read_extcsd_value(int fd, __u8 index, __u8 *value)
 {
	 __u8 ext_csd[512];
	 int ret;
	 
	 if (!value) {
		 return EMMC_ERROR_INVALID_PARAM;
	 }
	 
	 ret = read_extcsd(fd, ext_csd);
	 if (ret != EMMC_SUCCESS) {
		 return ret;
	 }
	 
	 *value = ext_csd[index];
	 return EMMC_SUCCESS;
 }
 
 /**
  * @brief 发送状态命令检查设备状态
  * 
  * @param fd 设备文件描述符
  * @return int 成功返回0，失败返回错误码
  */
 int mmc_send_status(int fd)
 {
	 int ret = 0;
	 struct mmc_ioc_cmd idata;
	 __u32 status;
	 
	 memset(&idata, 0, sizeof(idata));
	 idata.write_flag = 0;
	 idata.opcode = MMC_SEND_STATUS;
	 idata.arg = 0;  /* RCA通常为0 */
	 idata.flags = MMC_RSP_SPI_R2 | MMC_RSP_R1 | MMC_CMD_AC;
	 mmc_ioc_cmd_set_data(idata, &status);
 
	 ret = ioctl(fd, MMC_IOC_CMD, &idata);
	 if (ret) {
		 perror("ioctl MMC_SEND_STATUS failed");
		 return EMMC_ERROR_IOCTL;
	 }
	 
	 return EMMC_SUCCESS;
 }
 
 /**
  * @brief 验证扩展CSD数据有效性
  * 
  * @param ext_csd 扩展CSD数据
  * @return int 有效返回0，无效返回错误码
  */
 int validate_extcsd(const __u8 *ext_csd)
 {
	 if (!ext_csd) {
		 return EMMC_ERROR_INVALID_PARAM;
	 }
	 
	 /* 检查扩展CSD版本 */
	 if (ext_csd[EXT_CSD_EXT_CSD_REV] < 1) {
		 fprintf(stderr, "Unsupported EXT_CSD revision: %d\n", 
				 ext_csd[EXT_CSD_EXT_CSD_REV]);
		 return -EPERM;
	 }
	 
	 /* 检查设备类型 */
	 __u8 device_type = ext_csd[EXT_CSD_DEVICE_TYPE];
	 if (device_type == 0) {
		 fprintf(stderr, "Invalid device type\n");
		 return -EINVAL;
	 }
	 
	 return EMMC_SUCCESS;
 }
 
 /**
  * @brief 打印扩展CSD信息
  * 
  * @param ext_csd 扩展CSD数据
  */
 void print_extcsd_info(const __u8 *ext_csd)
 {
	 if (!ext_csd) {
		 return;
	 }
	 
	 printf("=== eMMC Extended CSD Information ===\n");
	 printf("Extended CSD Revision: %d\n", ext_csd[EXT_CSD_EXT_CSD_REV]);
	 printf("Device Type: 0x%02x\n", ext_csd[EXT_CSD_DEVICE_TYPE]);
	 
	 /* 计算设备容量 */
	 if (ext_csd[EXT_CSD_EXT_CSD_REV] >= 5) {
		 __u64 sectors = (ext_csd[215] << 24) | 
						 (ext_csd[214] << 16) | 
						 (ext_csd[213] << 8) | 
						 ext_csd[212];
		 printf("Sectors Count: %llu\n", (unsigned long long)sectors);
		 printf("Capacity: %.2f GB\n", 
				(sectors * 512.0) / (1024 * 1024 * 1024));
	 }
	 
	 printf("Partitioning Support: 0x%02x\n", 
			ext_csd[EXT_CSD_PARTITIONING_SUPPORT]);
	 printf("Boot Partition Size Multiplier: %d\n", 
			ext_csd[EXT_CSD_BOOT_SIZE_MULT]);
	 
	 /* RPMB相关 */
	 if (ext_csd[EXT_CSD_RPMB_SIZE_MULT] > 0) {
		 printf("RPMB Size Multiplier: %d\n", 
				ext_csd[EXT_CSD_RPMB_SIZE_MULT]);
	 }
	 printf("eMMC Life Time Estimation A [EXT_CSD_DEVICE_LIFE_TIME_EST_TYP_A]: 0x%02x\n",
		ext_csd[EXT_CSD_DEVICE_LIFE_TIME_EST_TYP_A]);
	printf("eMMC Life Time Estimation B [EXT_CSD_DEVICE_LIFE_TIME_EST_TYP_B]: 0x%02x\n",
		ext_csd[EXT_CSD_DEVICE_LIFE_TIME_EST_TYP_B]);
	 printf("=====================================\n");
 }
 
 /**
  * @brief 打开eMMC设备
  * 
  * @param device_path 设备路径
  * @return int 成功返回文件描述符，失败返回-1
  */
 int open_emmc_device(const char *device_path)
 {
	 int fd;
	 
	 if (!device_path) {
		 fprintf(stderr, "Device path is NULL\n");
		 return -1;
	 }
	 
	 fd = open(device_path, O_RDWR);
	 if (fd < 0) {
		 perror("Failed to open eMMC device");
		 return -1;
	 }
	 
	 return fd;
 }
 
 /**
  * @brief 安全写入扩展CSD寄存器（带验证）
  * 
  * @param fd 设备文件描述符
  * @param index 寄存器索引
  * @param value 要写入的值
  * @param verify 是否验证写入结果
  * @return int 成功返回0，失败返回错误码
  */
 int safe_write_extcsd_value(int fd, __u8 index, __u8 value, int verify)
 {
	 int ret;
	 __u8 read_value;
	 
	 /* 1. 检查设备状态 */
	 ret = mmc_send_status(fd);
	 if (ret != EMMC_SUCCESS) {
		 fprintf(stderr, "Device not ready before write\n");
		 return ret;
	 }
	 
	 /* 2. 写入寄存器 */
	 ret = write_extcsd_value(fd, index, value);
	 if (ret != EMMC_SUCCESS) {
		 fprintf(stderr, "Failed to write EXT_CSD[%d] = 0x%02x\n", index, value);
		 return ret;
	 }
	 
	 /* 3. 验证写入结果 */
	 if (verify) {
		 usleep(10000);  /* 等待10ms让设备稳定 */
		 
		 ret = read_extcsd_value(fd, index, &read_value);
		 if (ret != EMMC_SUCCESS) {
			 fprintf(stderr, "Failed to read back EXT_CSD[%d]\n", index);
			 return ret;
		 }
		 
		 if (read_value != value) {
			 fprintf(stderr, "Verify failed: wrote 0x%02x, read back 0x%02x\n", 
					 value, read_value);
			 return EMMC_ERROR_CMD_FAILED;
		 }
		 
		 printf("Successfully wrote and verified EXT_CSD[%d] = 0x%02x\n", 
				index, value);
	 }
	 
	 return EMMC_SUCCESS;
 }
 
 /**
  * @brief 获取内核版本信息
  */
 void print_kernel_version(void)
 {
 #ifdef __linux__
	 FILE *fp;
	 char buffer[1024];
	 
	 fp = fopen("/proc/version", "r");
	 if (fp) {
		 if (fgets(buffer, sizeof(buffer), fp)) {
			 printf("Kernel version: %s", buffer);
		 }
		 fclose(fp);
	 }
 #endif
 }
 
 /**
  * @brief 示例：读取并解析扩展CSD
  */
 int main_example(int argc, char *argv[])
 {
	 int fd;
	 __u8 ext_csd[512];
	 const char *device_path;
	 
	 if (argc < 2) {
		 fprintf(stderr, "Usage: %s <device_path>\n", argv[0]);
		 fprintf(stderr, "Example: %s /dev/mmcblk0\n", argv[0]);
		 return EXIT_FAILURE;
	 }
	 
	 device_path = argv[1];
	 
	 /* 打印内核版本信息 */
	 print_kernel_version();
	 
	 /* 打开设备 */
	 fd = open_emmc_device(device_path);
	 if (fd < 0) {
		 return EXIT_FAILURE;
	 }
	 
	 /* 读取扩展CSD */
	 if (read_extcsd(fd, ext_csd) != EMMC_SUCCESS) {
		 close(fd);
		 return EXIT_FAILURE;
	 }
	 
	 /* 验证数据 */
	 if (validate_extcsd(ext_csd) != EMMC_SUCCESS) {
		 close(fd);
		 return EXIT_FAILURE;
	 }
	 
	 /* 打印信息 */
	 print_extcsd_info(ext_csd);
	
	 
	 close(fd);
	 return EXIT_SUCCESS;
 }
 

 int main(int argc, char *argv[]) {
    printf("启动 Netlink 守护进程 (PID: %d)\n", getpid());
    main_example(argc, argv);
    // 守护进程化
    // become_daemon();
    
    // 设置信号处理
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    
    // 初始化 Netlink
    if (init_netlink() < 0) {
        printf("初始化 Netlink 失败\n");
        return 1;
    }
    
    printf("Netlink 初始化成功\n");
    
    // 主循环
    int counter = 0;
    while (running) {
        char msg[100];
        
        // 每3秒发送一次消息
        if (counter % 5 == 0) {
            snprintf(msg, sizeof(msg), "messsge #%d", counter/3);
            send_msg(msg);
        }
        
        // 接收消息
        recv_msg();
        
        sleep(1);
        counter++;
    }
    
    // 清理
    if (nl_fd >= 0) close(nl_fd);
    printf("程序退出\n");
    
    return 0;
}
