############################################################################
#
# common.make:
# This file contains all the common stuff used by MicroMonitor port-specific
# make files.

############################################################################
#
# Various relative directories used to build the monitor:
#
BASE		= $(TOPDIR)/target
CPUDIR		= $(BASE)/cpu/$(CPUTYPE)
ZLIBDIR		= $(BASE)/zlib
GLIBDIR		= $(BASE)/glib
COMDIR		= $(BASE)/common
MAKEDIR		= $(BASE)/make
DEVDIR		= $(BASE)/dev
FSDIR		= $(BASE)/fs
FATFSDIR	= $(FSDIR)/elmfatfs
JFFS2DIR	= $(BASE)/fs/jffs2

ifeq ($(TOOLBIN),)
TOOLBIN		= $(TOPDIR)/host/bin
endif

############################################################################
#
# Generic tools used by all make files:
# Names are derived from the TOOL_PREFIX variable
# assumed to be established in the make file that
# includes this.
#

ifeq ($(TOOL_PREFIX),)
TOOL_PREFIX		= $(CPUTYPE)-$(FILETYPE)
endif
NM				= $(TOOL_PREFIX)-nm
AR				= $(TOOL_PREFIX)-ar
LD				= $(TOOL_PREFIX)-ld
ASM				= $(TOOL_PREFIX)-as
CC				= $(TOOL_PREFIX)-gcc
STRIP			= $(TOOL_PREFIX)-strip
OBJCOPY			= $(TOOL_PREFIX)-objcopy
OBJDUMP			= $(TOOL_PREFIX)-objdump

LIBGCC			= `$(CC) --print-libgcc-file-name`
LIBDIR			= $(LIBGCC:/libgcc.a=)
NOF_LIBGCC		= $(LIBDIR)/nof/libgcc.a

# FLASHSUBDIR:
# This is actually a portion of the PATH to the flash source files.
# If not specified in the port-specific makefile, then assign the
# default type of "devices".  This then makes the assumption that
# the files specified by the FLASHSRC variable in the port-specific
# makefile are under $(BASE)/flash/devices (the new style flash
# driver for uMon).  For the older drivers, use the appropriate
# directory found under "boards" in the umon_main tree.
# The prefix $(BASE)/flash is assumed.
#
ifeq ($(FLASHSUBDIR),)
FLASHSUBDIR	= devices
endif

ifeq ($(FLASHDIR),)
FLASHDIR	= $(BASE)/flash/$(FLASHSUBDIR)
endif

# PORTDIR:
# By default, PORTDIR is umon/umon_ports; however some ports are not part
# of the CVS distribution; hence, umon/umon_ports sometimes needs to be
# overridden...
ifeq ($(PORTDIR),)
PORTDIR	= umon/umon_ports
endif

############################################################################
#
# Miscellaneous variables used by all targets:
#
DEPEND			= depend
DEPENDFILE		= depends
BUILDDIR		= build_$(PLATFORM)

COMMON_INCLUDE	= -I. -I$(COMDIR) -I$(CPUDIR) -I$(FLASHDIR) -I$(DEVDIR) \
				  -I$(ZLIBDIR) $(PORT_INCLUDE)

COMMON_CFLAGS	= -g -c -Wall -DPLATFORM_$(PLATFORM)=1 \
				  -fno-builtin -fno-toplevel-reorder

COMMON_AFLAGS	= -xassembler-with-cpp -c -D PLATFORM_$(PLATFORM)=1 \
				  -D ASSEMBLY_ONLY

CFLAGS			= $(COMMON_CFLAGS) $(CUSTOM_CFLAGS) $(COMMON_INCLUDE) \
				  $(CUSTOM_INCLUDE)
ASMFLAGS		= $(COMMON_AFLAGS) $(CUSTOM_AFLAGS) $(COMMON_INCLUDE) \
				  $(CUSTOM_INCLUDE)


MAKE_LDFILE		= $(TOOLBIN)/vsub $(PLATFORM)_$(@F:.$(FILETYPE)=.ldt) \
				  $(PLATFORM)_$(@F:.$(FILETYPE)=.ld)
MAKE_MONBUILT	= $(CC) $(COMMON_CFLAGS) $(CUSTOM_CFLAGS) \
					$(COMMON_INCLUDE) -omonbuilt.o $(COMDIR)/monbuilt.c
MAKE_GNUSYMS	= $(NM) --numeric-sort $@ > $(@:.$(FILETYPE)=.gsym)
DUMP_MAP		= $(OBJDUMP) -fh $@
MAKE_BINARY		= $(OBJCOPY) -O binary $@ $(@:.$(FILETYPE)=.bin)
LINK			= $(LD) -nostartfiles -o $@ \
				  -T $(PLATFORM)_$(@F:.$(FILETYPE)=.ld)
DISASSEMBLE		= $(OBJDUMP) --source --disassemble $@> \
				  $(@:.$(FILETYPE)=.dis)

MAKE_CTAGS		= ctags --file-tags=yes -n -L cscope.files
MAKE_MONSYMS	= $(TOOLBIN)/monsym -p0x -Pmm_ -Sx $(@:.$(FILETYPE)=.gsym) > \
				  $(@:.$(FILETYPE)=.usym)
DUMP_MAP_ALT	= $(TOOLBIN)/$(FILETYPE) -m $@
MAKE_BINARY_ALT	= $(TOOLBIN)/$(FILETYPE) -B $(@:.$(FILETYPE)=.bin) $@

############################################################################
#
# Various source lists used to build libraries:
#
FATFSSRC	= ff.c ffcmd.c cc932.c

ZLIBSRC		= adler32.c gzio.c infblock.c infcodes.c inffast.c inflate.c \
			  inftrees.c infutil.c trees.c uncompr.c zcrc32.c zutil.c

GLIBSRC		= abs.c asctime.c atoi.c crc16.c crc32.c div.c \
			  getopt.c inrange.c ldiv.c memccpy.c memchr.c \
			  memcmp.c memcpy.c memset.c pollconsole.c prascii.c printmem.c \
			  smemcpy.c smemset.c strcat.c strchr.c strcasecmp.c \
			  strcmp.c strcpy.c strlen.c strncat.c strncmp.c \
			  strncpy.c strpbrk.c strrchr.c strstr.c strtok.c \
			  strtol.c strtoul.c strtolower.c swap.c ticktock.c
