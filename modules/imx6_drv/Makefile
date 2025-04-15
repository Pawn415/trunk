KERNELDIR := ../../kernel_sdk/
CURRENT_PATH := $(shell pwd)

obj-m := misc_drv.o

build: kernel_modules

kernel_modules:
	arm-linux-gnueabihf-gcc misc_app.c -o misc_app
	make -C $(KERNELDIR) M=$(CURRENT_PATH) modules

clean:
	rm misc_app
	make -C $(KERNELDIR) M=$(CURRENT_PATH) clean
