# uboot

## uboot 官网
>http://www.denx.de/wiki/U-Boot/

## uboot烧录
`chmod 777 imxdownload`
`./imxdownload u-boot.bin /dev/sdd`

## uboot操作指令
### 帮助命令
`? bootz 或 help bootz`

### 信息查询命令

1. bdinfo       板子信息
2. printenv     环境信息
3. version      uboot版本和编译时间

### 环境变量操作
#### 修改环境变量
`setenv bootdelay 5`
`saveenv`

bootcmd
bootargs
`setenv bootargs 'console=ttymxc0,115200 root=/dev/mmcblk1p2 rootwait rw'`

#### 新建环境变量
setenv author zuozhongkai
saveenv

#### 删除环境变量
setenv author
saveenv

### 内存操作

操作Dram
1. md 命令用于显示内存值

md[.b, .w, .l] address [# of objects] 宽度  长度数据

md.b 80800000 14
md.w 80800000 14
md.l 80800000 14



2. nm 命令用于修改指定地址的内存值，命令格式如下：

nm [.b, .w, .l] address
`nm.l 80000000`

```c
=> nm.l 80000000 
80000000: 3fc0f414 ? 0x12345678 
80000000: 12345678 ? q 
```

3. mm 命令也是修改指定地址内存值的，使用 mm 修改内存值的时候地址会自增，而使用命
令 nm 的话地址不会自增

4. 命令 mw 用于使用一个指定的数据填充一段内存，命令格式如下：
mw [.b, .w, .l] address value [count]
mw.l 80000000 0A0A0A0A 10
比如使用.l 格式将以 0X80000000 为起始地址的 0x10 个内存块(0x10 * 4=64 字节)填充为 0X0A0A0A0A，命令如下：
mw.l 80000000 0A0A0A0A 10


5. cp 是数据拷贝命令，用于将 DRAM 中的数据从一段内存拷贝到另一段内存中，或者把 Nor 
Flash 中的数据拷贝到 DRAM 中。命令格式如下：
`cp [.b, .w, .l] source target count`
我们使用.l 格式将 0x80000000 处的地址拷贝到 0X80000100 处，长度为 0x10 个
内存块(0x10 * 4=64 个字节)，命令如下所示：
`cp.l 80000000 80000100 10`


6. cmp 是比较命令，用于比较两段内存的数据是否相等，命令格式如下：
`cmp [.b, .w, .l] addr1 addr2 count`
我们使用.l 格式来比较 0x80000000 和 0X80000100 这两个地址数据是否相等，比较长度为 0x10 个内存块(16 * 4=64 个字节)，命令如下所示：
`cmp.l 80000000 80000100 10`







### 网络操作

#### 网络设置

1. setenv ipaddr 192.168.1.50
2. setenv ethaddr b8:ae:1d:01:00:00
3. setenv gatewayip 192.168.1.1
4. setenv netmask 255.255.255.0
5. dhcp 用于从路由器获取 IP 地址

#### nfs

nfs [loadAddress] [[hostIPaddr:]bootfilename]
将zImage拷贝到DRAM中
nfs 80800000 192.168.1.253:/home/zuozhongkai/linux/nfs/zImage


#### tftp

sudo apt-get install tftp-hpa tftpd-hpa
sudo apt-get install xinetd

mkdir /home/zuozhongkai/linux/tftpboot
chmod 777 /home/zuozhongkai/linux/tftpboot

/etc/xinetd.d/tftp


server tftp
{
socket_type = dgram
protocol = udp
wait = yes
user = root
server = /usr/sbin/in.tftpd
server_args = -s /home/zuozhongkai/linux/tftpboot/
disable = no
per_source = 11
cps = 100 2
flags = IPv4
}

sudo service tftpd-hpa start

打开/etc/default/tftpd-hpa 文件，将其修改为如下所示内容：

`# /etc/default/tftpd-hpa`

TFTP_USERNAME="tftp"
TFTP_DIRECTORY="/home/zuozhongkai/linux/tftpboot"
TFTP_ADDRESS=":69" 
TFTP_OPTIONS="-l -c -s"

sudo service tftpd-hpa restart

tftp 80800000 zImage

### EMMC SD卡操作

命令| 描述
---|---
mmc info| 输出 MMC 设备信息
mmc read| 读取 MMC 中的数据。
mmc wirte| 向 MMC 设备写入数据。
mmc rescan |扫描 MMC 设备。
mmc part |列出 MMC 设备的分区。
mmc dev |切换 MMC 设备。
mmc list |列出当前有效的所有 MMC 设备。
mmc hwpartition| 设置 MMC 设备的分区。
mmc bootbus…… |设置指定 MMC 设备的 BOOT_BUS_WIDTH 域的值。
mmc bootpart…… |设置指定 MMC 设备的 boot 和 RPMB 分区的大小。
mmc partconf…… |设置指定 MMC 设备的 PARTITION_CONFG 域的值。
mmc rst |复位 MMC 设备
mmc setdsr |设置 DSR 寄存器的值


4. mmc dev 0 //切换到 SD 卡，0 为 SD 卡，1 为 eMMC
5. mmc part //查看 EMMC 分区
6. mmc read 80800000 600 10 //EMMC 的第 1536(0x600)个块开始，读取 16(0x10)个块的数据到 DRAM 的0X80800000 地址处
7. mmc write 80800000 2 32E //地址 分区 大小
8. mmc erase blk# cnt






## Fat格式文件操作

1. fatinfo 命令用于查询指定 MMC 设备分区的文件系统信息
fatinfo <interface> [<dev[:part]>]
fatinfo mmc 1:1
mmc 1 指的是emmc设备  :1  指的是分区1 

2. fatls 命令用于查询 FAT 格式设备的目录和文件信息
fatls <interface> [<dev[:part]>] [directory]
fatls mmc 1:1

3. fstype 用于查看 MMC 设备某个分区的文件系统格式
fstype <interface> <dev>:<part>
fstype mmc 1:0
fstype mmc 1:1
fstype mmc 1:2

4. fatload 命令用于将指定的文件读取到 DRAM 中
fatload <interface> [<dev[:part]> [<addr> [<filename> [bytes [pos]]]]]
fatload mmc 1:1 80800000 zImage

#define CONFIG_FAT_WRITE /* 使能 fatwrite 命令 */
5. fatwirte 命令用于将 DRAM 中的数据写入到 MMC 设备中
fatwrite <interface> <dev[:part]> <addr> <filename> <bytes>
tftp 80800000 zImage             下载到dram
fatwrite mmc 1:1 80800000 zImage 6788f8  从dram拷贝到emmc


## EXT格式文件系统操作

ext4ls mmc 1:2



## boot

### bootz
1. bootz 从dram中启动
tftp 80800000 zImage
tftp 83000000 imx6ull-alientek-emmc.dtb
bootz 80800000 - 83000000


fatwrite mmc 1:1 80800000 zImage sizehex
fatwrite mmc 1:1 83000000 imx6ull-alientek-emmc.dtb sizehex

fatls 查看要下 EMMC 的分区 1 中有没有 Linux 镜像文件和设备树文件

fatload mmc 1:1 80800000 zImage
fatload mmc 1:1 83000000 imx6ull-alientek-emmc.dtb
bootz 80800000 - 83000000

### bootm

### boot





## 其他指令

1. go 命令用于跳到指定的地址处执行应用，命令格式如下：
go addr [arg ...]
tftp 87800000 printf.bin
go 87800000

2. run
run 命令用于运行环境变量中定义的命令，比如可以通过“run bootcmd”来运行 bootcmd 中的启动命令



3. mtest
mtest 命令是一个简单的内存读写测试命令
`mtest [start [end [pattern [iterations]]]]`
`mtest 80000000 80001000`



