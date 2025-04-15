# a 文件夹的 Makefile

# 目标文件
OBJ_FILES = main.o

# 目标可执行文件
TARGET = testapp

# 默认目标
all: $(TARGET)

# 生成目标文件
%.o: %.c
	gcc -c $< -o $@

# 生成可执行文件
$(TARGET): $(OBJ_FILES)
	gcc $(OBJ_FILES) -o $(TARGET)

# 清理生成的文件
clean:
	rm -f $(OBJ_FILES) $(TARGET)
