# 编译器及选项
CC       := arm-linux-gnueabihf-gcc
CFLAGS   := -O2 -Wall -std=c11 -D_POSIX_C_SOURCE=199309L
LDFLAGS  := 
DEST_DIR := /home/he/Desktop/linux/alpha/nfs
# 目标文件及可执行文件
SRC      := char_time.c
TARGET   := char_time

.PHONY: all clean

# 默认目标
all: $(TARGET)

# 链接生成可执行文件
$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	cp $(TARGET) $(DEST_DIR)
# 清理中间文件
clean:
	@rm -f $(TARGET)

