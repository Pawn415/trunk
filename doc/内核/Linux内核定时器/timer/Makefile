# Makefile for my_driver 
 
#定义头文件路径和编译器选项  
KERNELDIR := /data1/heqinglong5/work/01.char_drv/kernel/kernel_sdk
CROSS_COMPILE := arm-seev100-linux-gnueabihf-
CC := $(CROSS_COMPILE)gcc  
  
#定义目标对象和源文件  
obj-m := timer.o
 
#编译规则  
all:  
	make -C $(KERNELDIR) M=$(PWD) modules  
	@find $(PWD) -not -name 'timer.c' -and -not -name 'Makefile' -and -not -name 'timer.ko' -and -type f -delete 
clean:  
	make -C $(KERNELDIR) M=$(PWD) clean  
