
#################################################################
#  Makefile for HTTP Protocol 1.1 Implementation - http proxy/server/gateway
#  (c) 1999-2008 Ke Heng Zhong (Beijing, China)
#  Writen by ke hengzhong (kehengzhong@yahoo.com)
#################################################################

ROOT = ..
BINNAME = libhttp.a

http_inc = $(ROOT)/ejet
http_src = $(ROOT)/ejet

adif_inc = $(ROOT)/adif
adif_lib = $(ROOT)/adif

probe_inc = $(ROOT)/epump
probe_lib = $(ROOT)/epump

mysql_inc = /usr/include/mysql
mysql_lib = /usr/lib/mysql

berkdb_inc = /usr/local/bdb/include
berkdb_lib = /usr/local/bdb/lib

obj = $(ROOT)/obj
dbgobj = $(ROOT)/dbgobj
dst = $(ROOT)/ejet

bin = $(dst)/$(BINNAME)

#################################################################
#  Customization of the implicit rules

CC = gcc -funsigned-char
#CC = gcc -funsigned-char
IFLAGS = -I$(adif_inc) -I$(probe_inc) -I$(http_inc)
#DEFS = -D_DEBUG -D_debug
DEFS = -D_debug
CFLAGS = -g -fPIC  -Wall $(IFLAGS)
LIBS = -ladif -lprobe -lnsl -lm -lz -lpthread
ARLIBS = $(adif_lib)/libadif.a
#LIBS = -lsocket -lnsl -lm -lxml -lz
LFLAGS = -L/usr/lib -L/usr/local/lib -L$(adif_lib) -L$(probe_lib)

ifeq ($(MAKECMDGOALS), debug)
  CFLAGS = -fPIC -g -Wall $(DEFS) $(IFLAGS)
  bin = $(dst)/libhttpdbg.a
  LIBS = -ladifdbg -lprobedbg -lz -lrt -lnsl -lm  -lpthread
  ARLIBS = $(adif_lib)/libadifdbg.a
endif

ifeq ($(MAKECMDGOALS), so)
  CFLAGS = -fPIC -g -Wall $(IFLAGS)
endif


#################################################################
# Macro definition check

ifeq ($(shell test -e /usr/include/sys/epoll.h && echo 1), 1)
  DEFS += -DHAVE_EPOLL
else ifeq ($(shell test -e /usr/include/sys/event.h && echo 1), 1)
  DEFS += -DHAVE_KQUEUE
else
  DEFS += -DHAVE_SELECT
endif

ifeq ($(shell test -e /usr/include/sys/eventfd.h && echo 1), 1)
  DEFS += -DHAVE_EVENTFD
endif

ifeq ($(shell test -e /usr/include/openssl/ssl.h && echo 1), 1)
  #DEFS += -DHAVE_OPENSSL
  #LIBS += -lssl -lcrypto
endif


#################################################################
# Set long and pointer to 64 bits or 32 bits

ifeq ($(BITS),)
  CFLAGS += -m64
else ifeq ($(BITS),64)
  CFLAGS += -m64
else ifeq ($(BITS),32)
  CFLAGS += -m32
else ifeq ($(BITS),default)
  CFLAGS += 
else
  CFLAGS += $(BITS)
endif


#################################################################
# OS-specific definitions and flags

UNAME := $(shell uname)

ifeq ($(UNAME), Linux)
  DEFS += -DUNIX -D_LINUX_
endif

ifeq ($(UNAME), FreeBSD)
  DEFS += -DUNIX -D_FREEBSD_
  LIBS += -liconv
endif

ifeq ($(UNAME), Darwin)
  DEFS += -D_OSX_

  PKG_VERSO_LIB = $(PKGLIB).$(PKG_VER).dylib
  PKG_SONAME_LIB = $(PKGLIB).$(PKG_VER_MAJOR).dylib
  LD_SONAME=

  SOFLAGS += -install_name $(libdst)/$(PKGLIB).dylib
  SOFLAGS += -compatibility_version $(PKG_VER_MAJOR)
  SOFLAGS += -current_version $(PKG_VER)
endif

ifeq ($(UNAME), Solaris)
  DEFS += -DUNIX -D_SOLARIS_
endif
 

#################################################################
# Merge the rules

CFLAGS += $(DEFS)
LIBS += $(APPLIBS)
 




#################################################################
#  Customization of the implicit rules - BRAIN DAMAGED makes (HP)

AR = ar
ARFLAGS = rv
RANLIB = ranlib
RM = /bin/rm -f
COMPILE.c = $(CC) $(CFLAGS) -c
LINK = $(CC) $(CFLAGS) $(LFLAGS) -o
SOLINK = $(CC) $(CFLAGS) $(LFLAGS) -shared -o

#################################################################
#  Modules

cnfs = $(wildcard $(http_inc)/*.h)
sources = $(wildcard $(http_src)/*.c)
objs = $(patsubst $(http_src)/%.c,$(obj)/%.o,$(sources))
dbgobjs = $(patsubst $(http_src)/%.c,$(dbgobj)/%.o,$(sources))
sobin = $(patsubst %.a,%.so,$(bin))



#################################################################
#  Standard Rules

.PHONY: all clean debug show

all: clean $(bin)
debug: clean $(bin)
so: clean $(sobin)
clean: 
	$(RM) $(objs)
show:
	@echo $(bin)
	ls $(objs)

#################################################################
#  Additional Rules
#
#  target1 [target2 ...]:[:][dependent1 ...][;commands][#...]
#  [(tab) commands][#...]
#
#  $@ - variable, indicates the target
#  $? - all dependent files
#  $^ - all dependent files and remove the duplicate file
#  $< - the first dependent file
#  @echo - print the info to console
#
#  SOURCES = $(wildcard *.c *.cpp)
#  OBJS = $(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(SOURCES)))
#  CSRC = $(filter %.c,$(files))


$(sobin): $(objs) 
	$(SOLINK) $@ $? 

$(bin): $(objs) 
	$(AR) $(ARFLAGS) $@ $?
	$(RANLIB) $(RANLIBFLAGS) $@

$(obj)/%.o: $(http_src)/%.c $(cnfs)
	$(COMPILE.c) $< -o $@

