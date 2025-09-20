# 时间管理

## JIFFIES
jiffies比喻为**时钟滴答**
```c
<include/linux/jiffies>
#define __jiffy_data __attribute__((section(".data")))
extern u64_jiffy_data jiffies_64;
extern unsigned long volatile __jiffy_data jiffies
```
时钟滴答的频率由一个宏定义HZ决定，滴答中断1次jiffies++;时钟频率有100HZ、250HZ、300HZ、和1000HZ，1000HZ意味着1Ms来一次中断，1Ms jiffies++;类似于MCU中的systick

jiffies使用
```c
#include <linux/jiffies.h>
unsigned long j ,timestamp1,timestamp2;
j = jiffies;
timestamp1 = j+2*HZ;//未来的2S钟
timestamp2 = j+2*HZ/1000;//未来的2ms钟
```

## 时间比较
因为有溢出的可能性，所以需要比较两个时间点的前后
```c
time_after(a,b);//如果时间点a在时间点b之后，就返回ture   b----a---->ture
time_before(a,b);//如果时间点a在时间点b之前，就返回ture   a----b---->ture

time_after_eq(a,b);//类似于after,相等也返回ture   b----a---->ture
time_before_eq(a,b);//类似于before，相等也返回ture   a----b---->ture
```
具体使用
```c
#include <linux/jiffies.h>
int demo()
{
    unsigned long timeout = jiffies + 2*HZ;//2ms之后
    do_something();
    if(time_after(jiffies,timeout))
    {
        return task_timeout();
    }
}
```

# 时间转换
`jiffies_to_msecs(endtimes-starttimes)//将jiffies的时间转为ms`
```c
unsigned int jiffies_to_msecs(const unsigned long j);
unsigned int jiffies_to_usecs(const unsigned long j);
unsigned int msecs_to_jiffies(const unsigned long j);
unsigned int usecs_to_jiffies(const unsigned long j); 

```
## 长延时
长延时让出处理器
```c
delay1s()
{
    __set_current_state(TASK_UNINTERRUPIBLE);
    schedule_timeout(HZ);
}
设置为不能被信号打断，让出CPU，等1S钟后唤醒进程
```
## 短延时
```c
void mdelay(unsigned long msecs);//毫秒
void udelay(unsigned long usecs);//微秒
void ndelay(unsigned long nsecs);//纳秒 
```

## 内核定时器
`struct timer_list`
```c
unsigned long expires;//指定定时器到期时间
void *(fun)(unsigned long);//到时间回调函数
unsigned long data;//定时器对象中携带的数据，当定时器被调用时，内核把该成员作为实际参数传递给定时器函数，定时器函数将在中断上下文中执行
```
```c
关键函数
init_timer();

add_timer();
void __init init_timers(void);
void run_local_timers(void);
static void run_timer_softirq(struct softirq_action*h);
static inline void __run_timers(struct tvec_base*base);
del_timer();

```
`struct tvec_base//关键结构体`

## 内核定时器使用流程

```c
struct timer_list timer;
void function(unsigned long arg)
{
    //逻辑代码
    //需要周期运行的话就是用mod_timer,重新设置超时时间
    mod_timer(&dev->timertest, jiffies + msecs_to_jiffies(2000));//2000ms
}

/* 初始化函数 */
void init(void) 
{
    init_timer(&timer); /* 初始化定时器 */

    timer.function = function; /* 设置定时处理函数 */
    timer.expires=jffies + msecs_to_jiffies(2000);/* 超时时间 2 秒 */
    timer.data = (unsigned long)&dev; /* 将设备结构体作为参数 */

    add_timer(&timer); /* 启动定时器 */
}

/* 退出函数 */
void exit(void)
{
    del_timer(&timer); /* 删除定时器 */
    /* 或者使用 */
    del_timer_sync(&timer);
}
```