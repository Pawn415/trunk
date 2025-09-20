# 异步通知
阻塞 非阻塞 poll函数可以让应用程序获取驱动设备的状态
异步通知：驱动程序可以主动的发送信号通知应用程序目前状态

## 信号释放
驱动信号释放就是主动发送信号给应用程序
驱动支持异步通知模板
```c
驱动程序
struct _dev
{
    struct cdev cdev;
    struct fasync_struct * async_queue;//异步结构体指针
}

static int _fasync(int fd,struct file* filp,int mode)
{
    struct dev *dev = filp->private_data;
    return fasync_helper(fd,filp,mode,&dev->async_queue);
}

static ssize_t _write(struct filp *filp,const char __user *buf ,size_t count,loff_t *f_pos)
{
    struct dev *dev = filp->private_data;

    if(dev->async_queue)
    {
        kill_fasync(&dev->async_queue,SIGIO,POLL_IN);
    }
}

static int _release(struct inod * inode , struct file *filp)
{
_fasync(-1,filp,0);
}

---------------------------------------------------------
应用程序

#include "stdio.h"
#include "unistd.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "fcntl.h"
#include "stdlib.h"
#include "string.h"
#include "poll.h"
#include "sys/select.h"
#include "sys/time.h"
#include "linux/ioctl.h"
#include "signal.h"


static int fd = 0; /* 文件描述符 */

/*
* SIGIO 信号处理函数
* @param - signum : 信号值
* @return : 无
*/
static void sigio_signal_func(int signum)
{
int err = 0;
unsigned int keyvalue = 0;

err = read(fd, &keyvalue, sizeof(keyvalue));
if(err < 0) {
/* 读取错误 */
} else {
printf("sigio signal! key value=%d\r\n", keyvalue);
}
}

/*
* @description : main 主程序
* @param - argc : argv 数组元素个数
* @param - argv : 具体参数
* @return : 0 成功;其他 失败
*/
int main(int argc, char *argv[])
{
int flags = 0;
char *filename;

if (argc != 2) {
printf("Error Usage!\r\n");
return -1;
}

filename = argv[1];
fd = open(filename, O_RDWR);
if (fd < 0) {
printf("Can't open file %s\r\n", filename);
return -1;
}
-------------------------------------------
/* 设置信号 SIGIO 的处理函数 */
signal(SIGIO, sigio_signal_func);
fcntl(fd, F_SETOWN, getpid()); /* 将当前进程的进程号告诉给内核 */
flags = fcntl(fd, F_GETFL); /* 获取当前的进程状态 */
fcntl(fd, F_SETFL, flags | FASYNC);/* 设置进程启用异步通知功能 */ 
--------------------------------------------------------
while(1) {
sleep(2);
}

close(fd);
return 0;
}

```

# linux 异步IO
```c

异步读取
int aio_read(struct aiocb * aiocbp);
请求之后立即返回

异步写入
int aio_write(struct aiocb * aiocbp);
请求之后立即返回

aio_error(struct aiocb * aiocbp);
EINPROGRESS:说明请求尚未完成
ECANCELED:说明请求被应用程序取消
-1：发生错误

ssize_t aio_return(struct aiocb * aiocbp);
只有在aio_error()调用确定请求已经完成（）之后，才调用这个函数，
返回值等价于同步情况下的read() write()系统调用的返回值（传输字节错误就为负值）

int aio_suspend(const struct aiocb* const cblist[],int n,const struct timespec *timeout)
用来阻塞调用进程，直到异步请求完成为止。

int aio_cancel(int fd,struct aiocb* aiocbp)
aio_cancel()函数允许用户取消某个文件描述符执行的所有IO请求

int lio_listio(int mod ,struct aiocb *list[],int nent,struct sigevent*sig);
可用于发起多个传输，他使得用户可以在一个系统调用中启动大量的IO操作

```
**确实不太懂，后续继续研究。。。。。。。。。。。。**

