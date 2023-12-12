#################################################################
#  Makefile for Application Development Interface Fundamental - adif
#  (c) 1999-2008 Ke Heng Zhong (Beijing, China)
#  Writen by ke hengzhong (kehengzhong@yahoo.com)
#################################################################

ROOT = ..
BINNAME = libadif.a

adif_inc = $(ROOT)/adif
adif_src = $(ROOT)/adif

obj = $(ROOT)/obj
dbgobj = $(ROOT)/dbgobj
dst = $(ROOT)/adif

bin = $(dst)/$(BINNAME)
solib = $(ROOT)/bin

#################################################################
#  Customization of the implicit rules

CC = gcc

IFLAGS = -I$(adif_inc)

CFLAGS = -Wall -O3 -fPIC
LFLAGS = -L/usr/lib
LIBS = -lnsl -lm -lz -lpthread
SOFLAGS = $(LD_SONAME)

ifeq ($(MAKECMDGOALS), debug)
  DEFS += -D_DEBUG
  CFLAGS += -g
endif

ifeq ($(MAKECMDGOALS), so)
  CFLAGS += 
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
endif

ifeq ($(UNAME), Darwin)
  DEFS += -D_OSX_

  PKG_VERSO_LIB = $(PKGLIB).$(PKG_VER).dylib
  PKG_SONAME_LIB = $(PKGLIB).$(PKG_VER_MAJOR).dylib
  LD_SONAME=

  SOFLAGS += -install_name $(dst)/$(PKGLIB).dylib
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

cnfs = $(wildcard $(adif_inc)/*.h)
sources = $(wildcard $(adif_src)/*.c)
objs = $(patsubst $(adif_src)/%.c,$(obj)/%.o,$(sources))
dbgobjs = $(patsubst $(adif_src)/%.c,$(dbgobj)/%.o,$(sources))
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
#	@cp $(sobin) $(solib)
#	$(SOLINK) $@ $? $(LIBS)

$(bin): $(objs) 
	$(AR) $(ARFLAGS) $@ $?
	$(RANLIB) $(RANLIBFLAGS) $@

$(obj)/%.o: $(adif_src)/%.c $(cnfs)
	$(COMPILE.c) $< -o $@

