#################################################################
#  Makefile for edgesrv - Downsha Cloud Storage Edge Service Node
#  (c) 2011-2012 Beijing Downsha Information Technology Co., Ltd.
#  Writen by ke hengzhong (kehengzhong@yahoo.com) - 2012-03-10
#################################################################

ROOT = .
BINNAME = edgesrv
LIBNAME = edgesrv

edge_inc = $(ROOT)/edgesrv
edge_src = $(ROOT)/edgesrv

webif_inc = $(ROOT)/webif
webif_src = $(ROOT)/webif

appmgmt_inc = $(ROOT)/appmgmt
appmgmt_src = $(ROOT)/appmgmt

adif_inc = $(ROOT)/adif
adif_src = $(ROOT)/adif

probe_inc = $(ROOT)/epump
probe_src = $(ROOT)/epump

http_inc = $(ROOT)/ejet
http_src = $(ROOT)/ejet

secab_inc = $(ROOT)/secab
secab_src = $(ROOT)/secab


datamgmt_inc = $(ROOT)/datamgmt
datamgmt_src = $(ROOT)/datamgmt

main_src = $(ROOT)
main_inc = $(ROOT)

#redis_inc = /usr/local/include/hiredis 
#redis_lib = /usr/local/lib

rtmp_inc  = /usr/local/include/librtmp
rtmp_lib  = /usr/local/lib

#mysql_inc = /usr/include/mysql
#mysql_lib = /usr/lib64/mysql
#mysql_inc = /usr/local/mysql/include
#mysql_lib = /usr/local/mysql/lib

#berkdb_inc = /usr/local/bdb/include
#berkdb_lib = /usr/local/bdb/lib

obj = $(ROOT)/obj
dbgobj = $(ROOT)/dbgobj
dst = $(ROOT)/bin
libdst = $(ROOT)/bin

bin = $(dst)/$(BINNAME)
libbin = $(libdst)/lib$(LIBNAME)


#################################################################
#  Customization of the implicit rules

CC = gcc -funsigned-char
#CC = gcc
DEFS = -D_DEBUG 
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
IFLAGS1 = -I$(edge_inc) -I$(webif_inc) -I$(appmgmt_inc) -I$(http_inc) -I$(probe_inc) -I$(secab_inc) 
IFLAGS2 = -I$(datamgmt_inc) -I$(adif_inc)
CFLAGS = -g  -Wall -fPIC  $(IFLAGS1) $(IFLAGS2)
APPLIBS = -l$(LIBNAME)
#SYSLIBS = -lmysqlclient -ldb -liconv 
SYSLIBS = -lrt -lm -lpthread 
GDLIBS =  -lfreetype -liconv -ldl
#LIBS = -lsocket -lnsl -lm -lz
#LFLAGS = -L/usr/lib -L/usr/local/lib -L$(libdst) -L$(mysql_lib) -L$(berkdb_lib) -L$(redis_lib)
LFLAGS = -L/usr/lib -L/usr/local/lib -L$(libdst)  -L$(redis_lib) -L$(rtmp_lib)
ifeq ($(MAKECMDGOALS), debug)
  CFLAGS = -g -Wall -fPIC $(DEFS) $(IFLAGS1) -Ddebug $(IFLAGS2)
  bin = $(dst)/$(BINNAME).dbg
  libbin = $(libdst)/lib$(LIBNAME)dbg
  APPLIBS = -l$(LIBNAME)dbg
endif

LIBS = $(APPLIBS) $(SYSLIBS) $(GDLIBS) 

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
#  Customization of the implicit rules - BRAIN DAMAGED makes (HP)

AR = ar
ARFLAGS = rv
RANLIB = ranlib
RM = /bin/rm -f
COMPILE.c = $(CC) $(CFLAGS) $(DEFS) -c
LINK = $(CC) $(CFLAGS) $(LFLAGS) $(DEFS) -o
SOLINK = $(CC) $(CFLAGS) $(LFLAGS) $(DEFS) -shared -o

#################################################################
#  Modules


adif_incs = $(wildcard $(adif_inc)/*.h)
adif_sources = $(wildcard $(adif_src)/*.c)
adif_objs = $(patsubst $(adif_src)/%.c,$(obj)/%.o,$(adif_sources))

secab_incs = $(wildcard $(secab_inc)/*.h)
secab_sources = $(wildcard $(secab_src)/*.c)
secab_objs = $(patsubst $(secab_src)/%.c,$(obj)/%.o,$(secab_sources))

probe_incs = $(wildcard $(probe_inc)/*.h)
probe_sources = $(wildcard $(probe_src)/*.c)
probe_objs = $(patsubst $(probe_src)/%.c,$(obj)/%.o,$(probe_sources))

datamgmt_incs = $(wildcard $(datamgmt_inc)/*.h)
datamgmt_sources = $(wildcard $(datamgmt_src)/*.c)
datamgmt_objs = $(patsubst $(datamgmt_src)/%.c,$(obj)/%.o,$(datamgmt_sources))

http_incs = $(wildcard $(http_inc)/*.h)
http_sources = $(wildcard $(http_src)/*.c)
http_objs = $(patsubst $(http_src)/%.c,$(obj)/%.o,$(http_sources))

appmgmt_incs = $(wildcard $(appmgmt_inc)/*.h)
appmgmt_sources = $(wildcard $(appmgmt_src)/*.c)
appmgmt_objs = $(patsubst $(appmgmt_src)/%.c,$(obj)/%.o,$(appmgmt_sources))

webif_incs = $(wildcard $(webif_inc)/*.h)
webif_sources = $(wildcard $(webif_src)/*.c)
webif_objs = $(patsubst $(webif_src)/%.c,$(obj)/%.o,$(webif_sources))

edge_incs = $(wildcard $(edge_inc)/*.h)
edge_sources = $(wildcard $(edge_src)/*.c)
edge_objs = $(patsubst $(edge_src)/%.c,$(obj)/%.o,$(edge_sources))

main_objs = $(obj)/main.o 

libobjs =  $(adif_objs) $(datamgmt_objs) $(secab_objs) $(probe_objs) $(http_objs)
objs =  $(edge_objs) $(main_objs) $(webif_objs) $(appmgmt_objs)

dbgobjs = $(patsubst $(obj)/%.o,$(dbgobj)/%.o,$(objs))


#################################################################
#  Standard Rules

.PHONY: all clean solib staticlib debug

all: clean staticlib solib $(bin) 

debug: clean staticlib $(bin) 

clean: 
	$(RM) $(objs)

$(bin): $(objs) 
#	@(if ! access -rwx $(obj); then mkdir $(obj);fi)
	$(LINK) $@ $? $(LIBS)

show:
	@(if ! access -rwx $(dbgobj); then mkdir $(dbgobj);fi)
	lgcc -funsigned-char -g -Wall -fPIC -D_DEBUG  -I./sksedge -I./webif -I./appmgmt -I./ejet -I./epump -I./secab  -Ddebug -I./datamgmt -I./adif -c main.c -o obj/main.os $(objs)

solib:
	#make -C adif so
	#make -C secab so
	#make -C datamgmt so
	#make -C http so
	$(SOLINK) $(libbin).so $(libobjs) 

staticlib:
	#make -C adif $(MAKECMDGOALS)
	#make -C secab $(MAKECMDGOALS)
	#make -C epump $(MAKECMDGOALS)
	#make -C datamgmt $(MAKECMDGOALS)
	#make -C ejet $(MAKECMDGOALS)
	$(AR) $(ARFLAGS) $(libbin).a $(libobjs)
	$(RANLIB) $(RANLIBFLAGS) $(libbin).a

#################################################################
#  Additional Rules


$(obj)/%.o: $(main_src)/%.c 
	$(COMPILE.c) $< -o $@

$(obj)/%.o: $(edge_src)/%.c $(edge_incs)
	$(COMPILE.c) $< -o $@

$(obj)/%.o: $(appmgmt_src)/%.c $(appmgmt_incs)
	$(COMPILE.c) $< -o $@
 
$(obj)/%.o: $(webif_src)/%.c $(webif_incs)
	$(COMPILE.c) $< -o $@

$(obj)/%.o: $(probe_src)/%.c $(probe_incs)
	$(COMPILE.c) $< -o $@

	
