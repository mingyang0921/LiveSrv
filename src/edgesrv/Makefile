
#################################################################
#  Makefile for CDN Edge Node Implementation - edge node server 
#  (c) 1999-2012 Ke Heng Zhong (Beijing, China)
#  Writen by ke hengzhong (kehengzhong@yahoo.com)
#################################################################

ROOT = ..
BINNAME = libedge

edge_inc = $(ROOT)/edgesrv
edge_src = $(ROOT)/edgesrv

http_inc = $(ROOT)/ejet
http_lib = $(ROOT)/ejet

adif_inc = $(ROOT)/adif
adif_lib = $(ROOT)/adif

secab_inc = $(ROOT)/secab
secab_lib = $(ROOT)/secab

datamgmt_inc = $(ROOT)/datamgmt
datamgmt_lib = $(ROOT)/datamgmt

probe_inc = $(ROOT)/epump
probe_lib = $(ROOT)/epump

mysql_inc = /usr/local/mysql/include
mysql_lib = /usr/local/mysql/lib

berkdb_inc = /usr/local/bdb/include
berkdb_lib = /usr/local/bdb/lib

obj = $(ROOT)/obj
dbgobj = $(ROOT)/dbgobj
dst = $(ROOT)/edge

bin = $(dst)/$(BINNAME).a

#################################################################
#  Customization of the implicit rules

CC = gcc -funsigned-char
#CC = gcc -funsigned-char
IFLAGS1 = -I$(edge_inc) 
IFLAGS2 = -I$(adif_inc) -I$(secab_inc) -I$(datamgmt_inc) -I$(probe_inc) -I$(http_inc)
IFLAGS3 = -I$(mysql_inc) -I$(berkdb_inc)
DEFS = -D_DEBUG
CFLAGS = -Wall $(IFLAGS1) $(IFLAGS2) $(IFLAGS3)
LIBS = -lhttp -ladif -ldatamgmt -lsecab -lprobe -lmysql -ldb -lnsl -lm -lz -lpthread
#LIBS = -lsocket -lnsl -lm -lxml -lz
LFLAGS = -L/usr/lib -L/usr/local/lib -L$(adif_lib) -L$(secab_lib) -L$(datammt_lib) -L$(probe_lib) -L$(http_lib) -L$(mysql_lib) -L$(berkdb_lib)

ifeq ($(MAKECMDGOALS), debug)
  CFLAGS = -g -Wall $(DEFS) $(IFLAGS1) $(IFLAGS2) $(IFLAGS3)
  bin = $(dst)/$(BINNAME)dbg.a
  LIBS = -lmp4dbg -ladifdbg -ldatamgmtdbg -lprobedbg -lmysqlclient -ldb -lz -lrt  -lnsl -lm  -lpthread
endif

ifeq ($(MAKECMDGOALS), so)
  CFLAGS = -fPIC $(DEFS) $(IFLAGS1) $(IFLAGS2) $(IFLAGS3)
endif

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
endif
 
ifeq ($(UNAME), Darwin)
  DEFS += -DOSX
endif
 
ifeq ($(UNAME), Solaris)
  DEFS += -DUNIX -D_SOLARIS_
endif



#################################################################
#  Modules

#################################################################
#  Modules

cnfs = $(wildcard $(edge_inc)/*.h)
sources = $(wildcard $(edge_src)/*.c)
objs = $(patsubst $(edge_src)/%.c,$(obj)/%.o,$(sources))
dbgobjs = $(patsubst $(edge_src)/%.c,$(dbgobj)/%.o,$(sources))
sobin = $(patsubst %.a,%.so,$(bin))


#################################################################
#  Standard Rules

.PHONY: all clean debug show

all: clean $(bin)
so: clean $(sobin)
debug: clean $(bin)
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
#	$(SOLINK) $@ $? $(LIBS)

$(bin): $(objs) 
	$(AR) $(ARFLAGS) $@ $?
	$(RANLIB) $(RANLIBFLAGS) $@

$(obj)/%.o: $(edge_src)/%.c $(cnfs)
	$(COMPILE.c) $< -o $@

