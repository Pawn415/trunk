
#arm-linux-gnueabihf-gcc virt_App.c -o virt_App
CC = arm-linux-gnueabihf-gcc
CFLAGS = 
# CFLAGS = -Wall -Wextra -Werror
SRCS = a.c b.c virtApp.c
OBJS = $(SRCS:.c=.o)
TARGET = virtApp

GET_DATE := $(shell date +%F)
GET_TIME := $(shell date +%H:%M:%S)

$(TARGET): $(OBJS)
	echo $(OBJS)
	$(CC) -D MAKE_TIME=\"$(GET_DATE)\ $(GET_TIME)\" -D DEFINE_TEST -o$@ $^
	cp -f virtApp ../../nfs

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

.PHONY: clean
clean:
	rm -f $(OBJS) $(TARGET)
