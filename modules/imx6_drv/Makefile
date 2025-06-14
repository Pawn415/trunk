# 基本的Makefile示例
# 目标编译文件名
TARGET = oom

# 源文件
SRCS = oom.c

# 相应的相对的盘片名称
OBJS = $(SRCS:.c=.o)

# 指定交叉编译工具链
CROSS_COMPILE = arm-linux-gnueabihf-
CC = $(CROSS_COMPILE)gcc

# 编译选项
CFLAGS = -Wall -g

# 目标文件要备份到的目定目录
DEST_DIR = /home/he/Desktop/linux/alpha/nfs

# 默认目标
all: $(TARGET) copy

# 编译给扩展后编译文件
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# 备份到指定目录
copy:
	cp $(TARGET) $(DEST_DIR)/

# 个别的 .o 文件编译
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# 清理
clean:
	rm -f $(TARGET) $(OBJS)
	rm -rf $(DEST_DIR)
