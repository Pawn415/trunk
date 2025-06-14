KERNELDIR := ../../kernel/kernel_sdk
CURRENT_PATH := $(shell pwd)

# Names and filenames
MODULE_NAME := chrdevbase
obj-m := $(MODULE_NAME).o
MODULE_KO := $(MODULE_NAME).ko

APP_SRC := $(MODULE_NAME)App.c
APP_BIN := $(MODULE_NAME)App

# Destination for artifacts
DEST_DIR := /home/he/Desktop/linux/alpha/nfs

# Default target: build both module and app, then install
all: kernel_modules $(APP_BIN) install

# Build kernel module
kernel_modules:
	$(MAKE) -C "$(KERNELDIR)" M="$(CURRENT_PATH)" modules

# Build user-space application
$(APP_BIN): $(APP_SRC)
	$(CC) $(CFLAGS) -I$(KERNELDIR)/include -o $@ $<

# Install artifacts to DEST_DIR
install: kernel_modules $(APP_BIN)
	@mkdir -p $(DEST_DIR)
	cp $(MODULE_KO) $(DEST_DIR)
	cp $(APP_BIN) $(DEST_DIR)
	@echo "Artifacts installed to $(DEST_DIR)"

# Clean both module and application
clean:
	$(MAKE) -C "$(KERNELDIR)" M="$(CURRENT_PATH)" clean
	rm -f $(APP_BIN)
	rm -f $(MODULE_KO)
