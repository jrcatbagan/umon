###############################################################################
#
# MicroMonitor Release 1.0 template board makefile.
#
# This file can be used as a starting point for a new port makefile.
# Structurally, this makefile should work for all ports, so the majority
# of the changes will be to modify the content of variables already defined.
# If you are transitioning to release 1.0 of MicroMonitor from an earlier
# verion, refer to the OLD_TO_NEW.txt file for a set of conversion steps. 
#
# The VERY FIRST ERROR that this makefile is likely to generate will
# be due to the fact that the UMONTOP shell variable is either missing
# or not properly specified.  Make sure you start off by setting
# the UMONTOP variable to be set to the full path of the umon_main
# directory which contains all of the common source code for both target
# and host related builds for uMon.
#
# This template defaults to use a ppc-elf- GCC tool prefix.  Refer to
# CPUTYPE & FILETYPE variables below to change that.
#
###############################################################################
#
# Build Variables:
# TOPDIR:
#	Set to the content of UMONTOP, which is an externally defined
#	shell variable assumed by this environment to be set to the full
#	path of the umon_main directory.
# PLATFORM:
#	ASCII name of the target platform (e.g. "Cogent CSB472")
# TGTDIR:
#	The name of the working directory that this port is to be built in.
# CPUTYPE/FILETYPE:
#	This combination of variables builds the GCC prefix (and is used for
#	a few other purposes.
#	Typical values for CPUTYPE are arm, ppc, m68k, mips and xscale.
#	Typical values for FILETYPE are elf, coff, rtems and aout.
# CUSTOM_FLAGS:
#	Establish the custom portion of the 'C' flags used for cross-compilation.
#	Refer to the file $(UMONTOP)/target/make/common.make for the set of
#	variables used (in addition to this one) to build the final CFLAGS
#	variable.
# CUSTOM_AFLAGS:
#	Similar to CUSTOM_FLAGS, this is used for assembler files.
# CUSTOM_INCLUDE:
#	Used to specify port-specific additions to the INCLUDES list. 
#
PLATFORM		= TEMPLATE
TOPDIR			= $(UMONTOP)
TGTDIR			= template
CPUTYPE			= arm
FILETYPE		= elf
CUSTOM_CFLAGS	= 
CUSTOM_AFLAGS	=
CUSTOM_INCLUDE	=

# Using tools installed by "sudo apt-get install gcc-arm-none-eabi"...
ABIDIR          = /usr/lib/gcc/arm-none-eabi/4.8.2
TOOL_PREFIX     = /usr/bin/arm-none-eabi

# If using something other than the Microcross model for the GNU
# tools (i.e. CPUTYPE-FILETYPE-TOOL) then specify the tool prefix
# here... (for example, powerpc-405-linux-gnu)
#TOOL_PREFIX	=	powerpc-405-linux-gnu

###############################################################################
#
# Memory map configuration:
# The following variables are used to establish the system's memory map.
# The values associated with these variables are substituted into
# the .ldt (.ld template) files to generate the .ld files actually used
# for the final linkage.  This allows the user to override these defaults
# without touching a memory map file.  Adjust them appropriately based on
# the target memory map.
#
# BOOTRAMBASE/BOOTRAMLEN: 
# BOOTROMBASE/BOOTROMLEN: 
#	Specify the base and length of RAM and ROM (i.e. flash) space used by
#	the version of the monitor that will reside (and run out of) boot flash.
# RAMTSTBASE/RAMTSTLEN:
#	Specify the base and length of RAM to be used by the "test" version of
#	the monitor that will reside entirely in RAM.
BOOTRAMBASE=0x3000
BOOTRAMLEN=0x7ffff
BOOTROMBASE=0xfff80000
BOOTROMLEN=0x7ffff
RAMTSTBASE=0x200000
RAMTSTLEN=0x7ffff

# These next two hard-coded values are used by ram-based versions of
# uMon to allow them to know where these flash-based structures are located.
# Obviously the addresses are port-specific and are specified here for
# reference only.
MACADDRBASE=0xfff80000
ALTTFSDEVTBLBASE=0xfff80020


include	$(TOPDIR)/target/make/common.make

###############################################################################
#
# Build each variable from a list of individual filenames...
#
# LOCSSRC:
#	Local (in this directory) assembler files.
# LOCCSRC:
#	Local (in this directory) 'C' files.
#	Note regarding except_xxx.c and strace_xxx.c...
#	Prior to writing your processor-specific except_xxx.c and strace_xxx.c
#	check the target's cpu directory to make sure it isn't already available.
#	If available, then change the filenames accordingly and move them to
#	the CPUCSRC filelist.  If you need to develop them, build them in
#	this port-specific directory, then upon completion they can be moved
#	to the cpu-specific directory so that they can be used by other ports.
# CPUSSRC:
#	CPU-specific assembler files in the main/target/cpu/CPU directory.
# CPUSSRC:
#	CPU-specific 'C' files in the main/target/cpu/CPU directory.
# COMCSRC:
#	Core 'C' files found in the main/target/core directory.
# IODEVSRC:
#	Device-specific files found in main/target/dev directory.	
# FLASHSRC:
#	The flash driver file found in main/target/flash/devices directory.
#
LOCSSRC		= reset.S 
CPUSSRC		= 
LOCCSRC		= cpuio.c etherdev.c except_template.c strace_template.c
COMCSRC		= arp.c bbc.c cast.c cache.c chario.c cmdtbl.c crypt.c \
			  docmd.c dhcp_00.c dhcpboot.c edit.c ee.c env.c ethernet.c \
			  flash.c genlib.c icmp.c if.c ledit_vt100.c monprof.c \
			  mprintf.c memcmds.c malloc.c moncom.c memtrace.c misccmds.c \
			  misc.c password.c redirect.c reg_cache.c sbrk.c start.c \
			  struct.c symtbl.c tcpstuff.c tfs.c tfsapi.c tfsclean1.c \
			  tfscli.c \
			  tfsloader.c tfslog.c tftp.c timestuff.c xmodem.c gdb.c
CPUCSRC		= 
IODEVSRC	= 
FLASHSRC	= am29lv160d_16x1.c

include $(TOPDIR)/target/make/objects.make

OBJS	= 	$(LOCSOBJ) $(CPUSOBJ) $(LOCCOBJ) $(CPUCOBJ) $(COMCOBJ) \
			$(FLASHOBJ) $(IODEVOBJ)

#########################################################################
#
# Targets...

# boot:
# The default target is "boot", a shortcut to $(BUILDDIR)/boot.$(FILETYPE).
# This builds the bootflash image that can be used by 'newmon' to 
# load a new version onto an already running system.
#
boot:	$(BUILDDIR)/boot.$(FILETYPE)
	@echo Boot version of uMon built under $(BUILDDIR) ...
	@ls $(BUILDDIR)/boot*

# ramtst:
# A shortcut to $(BUILDDIR)/ramtst.$(FILETYPE).  This is a version of uMon
# that resides strictly in RAM and is used for two main purposes:
# 1. To test new monitor features prior to burning the boot flash.
# 2. To be downloaded into the RAM space of a board that has no programmed
#    boot flash.  This provides a running monitor that can then accept
#    an incoming bootflash image using 'newmon'.
#
ramtst:	$(BUILDDIR)/ramtst.$(FILETYPE)
	@echo Ram-resident test version of uMon built under $(BUILDDIR) ...
	@ls $(BUILDDIR)/ramtst*

$(BUILDDIR)/boot.$(FILETYPE): $(BUILDDIR) $(OBJS) libz.a libg.a makefile
	$(MAKE_MONBUILT)
	$(MAKE_LDFILE) \
		BOOTRAMBASE=$(BOOTRAMBASE) BOOTRAMLEN=$(BOOTRAMLEN) \
		BOOTROMBASE=$(BOOTROMBASE) BOOTROMLEN=$(BOOTROMLEN)
	$(LINK) -e coldstart $(OBJS) monbuilt.o libz.a libg.a $(LIBGCC)
	$(MAKE_BINARY)
	$(MAKE_GNUSYMS)

$(BUILDDIR)/ramtst.$(FILETYPE): $(BUILDDIR) $(OBJS) libz.a libg.a makefile
	$(MAKE_MONBUILT)
	$(MAKE_LDFILE) \
		RAMTSTBASE=$(RAMTSTBASE) RAMTSTLEN=$(RAMTSTLEN) \
		MACADDRBASE=$(MACADDRBASE) ALTTFSDEVTBLBASE=$(ALTTFSDEVTBLBASE)
	$(LINK) -e coldstart $(OBJS) monbuilt.o libz.a libg.a $(LIBGCC)
	$(MAKE_BINARY)
	$(MAKE_GNUSYMS)

include $(TOPDIR)/target/make/rules.make

#########################################################################
#
# Miscellaneous...
#
######
#
# cscope_local:
# Put additional files here that should be included in the cscope
# files list.  This is called before the generic cscope file builder,
# so it should create the cscope.files file.
#
cscope_local:
	echo $(CPUDIR)/regs_403.c >cscope.files

######
#
# help_local:
# Add text here as needed by the port.
#
help_local:
	@echo "This template defaults to using ppc-elf as the tool prefix."
	@echo "To override this default modify CPU & FILETYPE variables."
	@echo 
