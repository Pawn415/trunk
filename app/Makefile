# Top-level Makefile

# 目标文件夹
SUBDIRS = a b main

# 默认目标，编译所有子目录
all: $(SUBDIRS)

# 递归进入子目录并执行子目录的 Makefile
$(SUBDIRS):
	$(MAKE) -C $@

# 清理所有子目录生成的文件
clean:
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir clean; \
	done
