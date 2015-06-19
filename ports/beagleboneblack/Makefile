###############################################################################
#
# Beaglebone Black makefile
#
#
PLATFORM		= BEAGLEBONEBLACK
TOPDIR			= $(UMONTOP)
TGTDIR			= beagleboneblack
CPUTYPE			= arm
FILETYPE		= elf

# Using tools installed by "sudo apt-get install gcc-arm-none-eabi"...
ABIDIR          = /usr/lib/gcc/arm-none-eabi/4.8.2
LIBABIDIR       = -L $(ABIDIR) 
TOOL_PREFIX     = /usr/bin/arm-none-eabi

COMMON_AFLAGS	= -c -D PLATFORM_$(PLATFORM)=1 -D ASSEMBLY_ONLY 
CUSTOM_CFLAGS	= -mcpu=arm1136j-s -O2 -isystem $(ABIDIR)/include -Wno-char-subscripts


###############################################################################
#
# Memory map configuration:
# The following variables are used to establish the system's memory map.
# 
BOOTROMBASE=0x08000000
BOOTROMLEN=0x100000
BOOTRAMBASE=0x80000000
BOOTRAMLEN=0x100000
RAMTSTROMBASE=0x80100000
RAMTSTROMLEN=0x100000
ATAGSIZE=0x1000

# These next two hard-coded values are used by the ramtst version of
# uMon to allow it to know where these flash-based structures are located.
MACADDRBASE=0x08000020
ALTTFSDEVTBLBASE=0x08000040

include	$(TOPDIR)/make/common.make

# Build each variable from a list of individual filenames...
#
LOCSSRC		= 
CPUSSRC		= vectors_arm.S
LOCCSRC		= ad7843.c cpuio.c etherdev.c nand740.c omap3530_gpio.c \
			  omap3530_lcd.c omap3530_sdmmc.c
COMCSRC		= arp.c cast.c cache.c chario.c cmdtbl.c \
			  docmd.c dhcp_00.c dhcpboot.c dns.c edit.c env.c ethernet.c \
			  flash.c gdb.c icmp.c if.c ledit_vt100.c monprof.c \
			  fbi.c font.c mprintf.c memcmds.c malloc.c moncom.c memtrace.c \
			  misccmds.c misc.c nand.c password.c redirect.c \
			  reg_cache.c sbrk.c sd.c \
			  start.c struct.c symtbl.c syslog.c tcpstuff.c tfs.c tfsapi.c \
			  tfsclean1.c tfscli.c tfsloader.c tfslog.c tftp.c timestuff.c \
			  tsi.c xmodem.c
CPUCSRC		= ldatags.c except_arm.c misc_arm.c strace_arm.c 
IODEVSRC	= smsc911x.c uart16550.c fb_draw.c
FLASHSRC	= s29gl512n_16x1.c


include $(TOPDIR)/make/objects.make

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

$(BUILDDIR)/boot.$(FILETYPE): $(BUILDDIR) $(OBJS) libz.a \
		libg.a Makefile
	$(CC) $(ASMFLAGS) -o rom_reset.o rom_reset.S
	$(MAKE_MONBUILT)
	sed -e s/ROMBASE/$(BOOTROMBASE)/ -e s/ROMLEN/$(BOOTROMLEN)/ \
		-e s/DRAMBASE/$(BOOTRAMBASE)/ -e s/DRAMLEN/$(BOOTRAMLEN)/ -e s/ATAGSIZE/$(ATAGSIZE)/ \
		$(PLATFORM)_$(@F:.$(FILETYPE)=.ldt) > $(PLATFORM)_$(@F:.$(FILETYPE)=.ld)
	$(LINK) -e coldstart $(OBJS) monbuilt.o libz.a \
		libg.a $(LIBABIDIR) $(LIBGCC)
	$(MAKE_BINARY)
	$(MAKE_GNUSYMS)

$(BUILDDIR)/ramtst.$(FILETYPE): $(BUILDDIR) $(OBJS) libz.a \
		libg.a Makefile
	$(CC) $(ASMFLAGS) -o ram_reset.o ram_reset.S
	$(MAKE_MONBUILT)
	sed -e s/RAMTSTROMBASE/$(RAMTSTROMBASE)/ \
		-e s/RAMTSTROMLEN/$(RAMTSTROMLEN)/ -e s/ATAGSIZE/$(ATAGSIZE)/ \
		-e s/MACADDRBASE/$(MACADDRBASE)/ -e s/ALTTFSDEVTBLBASE/$(ALTTFSDEVTBLBASE)/ \
		$(PLATFORM)_$(@F:.$(FILETYPE)=.ldt) > $(PLATFORM)_$(@F:.$(FILETYPE)=.ld)

	$(LINK) -e coldstart $(OBJS) monbuilt.o libz.a libg.a $(LIBGCC)
	$(MAKE_BINARY)
	$(MAKE_GNUSYMS)

include $(TOPDIR)/make/rules.make


#########################################################################
#
# Miscellaneous...
cscope_local:
	ls rom_reset.S ram_reset.S >cscope.files
	ls $(FLASHDIR)/s29gl512n_16x1.c >>cscope.files
	ls $(FLASHDIR)/s29gl512n_16x1.h >>cscope.files

help_local:

varcheck:
