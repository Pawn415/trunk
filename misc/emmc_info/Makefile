CC = arm-linux-gnueabihf-gcc
AM_CFLAGS = -D_FILE_OFFSET_BITS=64 -D_FORTIFY_SOURCE=2
DEST_DIR := /home/he/Desktop/linux/alpha/nfs
CFLAGS ?= -g -O2
objects = \
	mmc.o \
	lsmmc.o \
	3rdparty/hmac_sha/hmac_sha2.o \
	3rdparty/hmac_sha/sha2.o

CHECKFLAGS = -Wall -Wuninitialized -Wundef

DEPFLAGS = -Wp,-MMD,$(@D)/.$(@F).d,-MT,$@

override CFLAGS := $(CHECKFLAGS) $(AM_CFLAGS) $(CFLAGS)

INSTALL = install
prefix ?= /home/he/Desktop/linux/alpha/nfs
LIBS=
RESTORE_LIBS=

progs = emmc_app_info

# make C=1 to enable sparse
ifdef C
	check = sparse $(CHECKFLAGS)
endif

all: $(progs) manpages
	cp $(progs) $(prefix)
.c.o:
ifdef C
	$(check) $<
endif
	$(CC) $(CPPFLAGS) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

mmc: $(objects)
	$(CC) $(CFLAGS) -o $@ $(objects) $(LDFLAGS) $(LIBS)

# manpages:
# 	$(MAKE) -C man

# install-man:
# 	$(MAKE) -C man install

clean:
	rm -f $(progs) $(objects)
	# $(MAKE) -C man clean

install: $(progs)
	cp $(progs) $(prefix)

-include $(foreach obj,$(objects), $(dir $(obj))/.$(notdir $(obj)).d)

.PHONY: all clean install manpages
