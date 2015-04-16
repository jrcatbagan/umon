###########################################################################
#
# targets.make:
# Targets used by all target builds.
#
-include $(DEPENDFILE)

$(LOCSOBJ):	
	$(CC) $(ASMFLAGS) -o ${@F} $(@:%.o=%.S)

$(LOCCOBJ):
	$(CC) $(CFLAGS) -o ${@F} $(@:%.o=%.c)

$(CPUSOBJ):
	$(CC) $(ASMFLAGS) -o ${@F} $(CPUDIR)/$(@:%.o=%.S)

$(CPUCOBJ):
	$(CC) $(CFLAGS) -o ${@F} $(CPUDIR)/$(@:%.o=%.c)

$(COMCOBJ):
	$(CC) $(CFLAGS) -o ${@F} $(COMDIR)/$(@:%.o=%.c)

$(IODEVOBJ):
	$(CC) $(CFLAGS) -o ${@F} $(DEVDIR)/$(@:%.o=%.c)

$(FLASHOBJ):
	$(CC) $(CFLAGS) -o ${@F} $(FLASHDIR)/$(@:%.o=%.c)

$(ZLIBOBJ):	
	$(CC) $(CFLAGS) -o ${@F} $(ZLIBDIR)/$(@:%.o=%.c)

$(GLIBOBJ):	
	$(CC) $(CFLAGS) -o ${@F} $(GLIBDIR)/$(@:%.o=%.c)

$(FATFSOBJ):	
	$(CC) $(CFLAGS) -o ${@F} $(FATFSDIR)/$(@:%.o=%.c)

libg.a:	$(GLIBOBJ)
	$(AR) rc libg.a $(GLIBOBJ)

libz.a:	$(ZLIBOBJ)
	$(AR) rc libz.a $(ZLIBOBJ)

libfatfs.a:	$(FATFSOBJ)
	$(AR) rc libfatfs.a $(FATFSOBJ)

###########################################################################
#
# depend:
# Build a file of dependencies to be included in the main makefile.
#
$(DEPEND):	
	@if [ ! -f $(DEPENDFILE) ] ; \
	then \
		for obj in $(LOCSSRC); do \
			$(CC) $(COMMON_AFLAGS) $(CUSTOM_AFLAGS) $(COMMON_INCLUDE) \
				$(CUSTOM_INCLUDE) -MM $$obj >>$(DEPENDFILE) ; \
			echo $$obj dependencies... ; \
		done; \
		for obj in $(LOCCSRC); do \
			$(CC) $(COMMON_CFLAGS) $(CUSTOM_CFLAGS) $(COMMON_INCLUDE) \
				$(CUSTOM_INCLUDE) -MM $$obj >>$(DEPENDFILE) ; \
			echo $$obj dependencies... ; \
		done; \
		for obj in $(CPUSSRC); do \
			$(CC) $(COMMON_AFLAGS) $(CUSTOM_AFLAGS) $(COMMON_INCLUDE) \
				$(CUSTOM_INCLUDE) -MM $(CPUDIR)/$$obj >>$(DEPENDFILE) ; \
			echo $$obj dependencies... ; \
		done; \
		for obj in $(CPUCSRC); do \
			$(CC) $(COMMON_CFLAGS) $(CUSTOM_CFLAGS) $(COMMON_INCLUDE) \
				$(CUSTOM_INCLUDE) -MM $(CPUDIR)/$$obj >>$(DEPENDFILE) ; \
			echo $$obj dependencies... ; \
		done; \
		for obj in $(COMCSRC); do \
			$(CC) $(COMMON_CFLAGS) $(CUSTOM_CFLAGS) $(COMMON_INCLUDE) \
				$(CUSTOM_INCLUDE) -MM $(COMDIR)/$$obj >>$(DEPENDFILE) ; \
			echo $$obj dependencies... ; \
		done; \
		for obj in $(IODEVSRC); do \
			$(CC) $(COMMON_CFLAGS) $(CUSTOM_CFLAGS) $(COMMON_INCLUDE) \
				$(CUSTOM_INCLUDE) -MM $(DEVDIR)/$$obj >>$(DEPENDFILE) ; \
			echo $$obj dependencies... ; \
		done; \
		for obj in $(FLASHSRC); do \
			$(CC) $(COMMON_CFLAGS) $(CUSTOM_CFLAGS) $(COMMON_INCLUDE) \
				$(CUSTOM_INCLUDE) -MM $(FLASHDIR)/$$obj >>$(DEPENDFILE) ; \
			echo $$obj dependencies... ; \
		done; \
		for obj in $(ZLIBSRC); do \
			$(CC) $(COMMON_CFLAGS) $(CUSTOM_CFLAGS) $(COMMON_INCLUDE) \
				$(CUSTOM_INCLUDE) -MM $(ZLIBDIR)/$$obj >>$(DEPENDFILE) ; \
			echo $$obj dependencies... ; \
		done; \
		for obj in $(GLIBSRC); do \
			$(CC) $(COMMON_CFLAGS) $(CUSTOM_CFLAGS) $(COMMON_INCLUDE) \
				$(CUSTOM_INCLUDE) -MM $(GLIBDIR)/$$obj >>$(DEPENDFILE) ; \
			echo $$obj dependencies... ; \
		done; \
	fi


###########################################################################
#
# builddir:
# All output files that are not just .o's or .a's are put in the 
# build directory, so make sure that directory exists...
#
$(BUILDDIR):
	@if ! test -d $(BUILDDIR) ; then rm -f $(BUILDDIR); mkdir $(BUILDDIR); fi

###########################################################################
#
# clobber:
# Remove all files built by this makefile...
#
clobber:
	rm -f *.o *.a *.ld
	rm -rf $(BUILDDIR) tags cscope* *.gz *.ld depends

###########################################################################
#
# clean:
# Remove all the object files that were built by this makefile...
#
clean:
	rm -f *.o *.a

###########################################################################
# Added on 6-6-05, to make boot and ramtst files with only one command
# Top level target: all
#
all:	boot ramtst

###########################################################################
#
# ctags:
# Build tags file for use by various editors.
#
ctags:
	@if ! test -f cscope.files ; then make cscope; fi
	$(MAKE_CTAGS)

###########################################################################
#
# cscope:
# Build 'cscope.files' file, which consists of all source files 
# that make up the build.  It is used by the cscope tool.
#
cscope: cscope_local
	for file in $(LOCSOBJ:%.o=%.S); do ls $$file \
		>>cscope.files; done 
	for file in $(LOCCOBJ:%.o=%.c); do ls $$file \
		>>cscope.files; done 
	for file in $(CPUSOBJ:%.o=%.S); do ls $(CPUDIR)/$$file \
		>>cscope.files; done 
	for file in $(CPUCOBJ:%.o=%.c); do ls $(CPUDIR)/$$file \
		>>cscope.files; done 
	for file in $(COMCOBJ:%.o=%.c); do ls $(COMDIR)/$$file \
		>>cscope.files; done 
	for file in $(IODEVOBJ:%.o=%.c); do ls $(DEVDIR)/$$file \
		>>cscope.files; done 
	for file in $(GLIBOBJ:%.o=%.c); do ls $(GLIBDIR)/$$file \
		>>cscope.files; done 
	for file in $(ZLIBOBJ:%.o=%.c); do ls $(ZLIBDIR)/$$file \
		>>cscope.files; done 
	ls $(COMDIR)/monbuilt.c >>cscope.files
	ls *.h >>cscope.files
	ls $(CPUDIR)/*.h >>cscope.files
	ls $(COMDIR)/*.h >>cscope.files
	ls $(ZLIBDIR)/*.h >>cscope.files

###########################################################################
#
# libgcc:
# Dump the current libgcc library 
#
libgcc:
	@$(CC) --print-libgcc-file-name


###########################################################################
#
# buildcheck:
# Check for the presence of the BUILD variable and $(BUILDDIR) directory.
#
buildcheck:
ifeq ($(BUILD),)
	@echo Must set BUILD variable.
	@exit 1
endif
	@if ! test -d $(BUILDDIR) ; \
	then \
		echo Directory $(BUILDDIR) doesn\'t exist; \
		exit 1; \
	fi

###########################################################################
#
# monsym:
# Build the symbol table file used by the monitor
#
monsym:	buildcheck
	@if ! test -f $(BUILDDIR)/$(BUILD).gsym ; \
	then \
		echo File $(BUILDDIR)/$(BUILD).gsym doesn\'t exist; \
		exit 1; \
	fi
	$(TOOLBIN)/monsym -Pmm_ -Sx $(BUILDDIR)/$(BUILD).gsym \
		> $(BUILDDIR)/$(BUILD).usym

###########################################################################
#
# strip:
# Remove symbols from ramtst.elf image.
#
strip:
	@if ! test -f $(BUILDDIR)/ramtst.elf ; \
	then \
		echo File $(BUILDDIR)/ramtst.elf doesn\'t exist; \
		exit 1; \
	fi
	$(STRIP) $(BUILDDIR)/ramtst.elf
	
###########################################################################
#
# map:
# Dump a map of the specified build using objdump.
#
map:	buildcheck
	@if ! test -f $(BUILDDIR)/$(BUILD).elf ; \
	then \
		echo File $(BUILDDIR)/$(BUILD).elf doesn\'t exist; \
		exit 1; \
	fi
	$(OBJDUMP) -fh $(BUILDDIR)/$(BUILD).elf

###########################################################################
#
# emap:
# Dump a map of the specified build using the 'elf' tool.
#
emap:	buildcheck
	@if ! test -f $(BUILDDIR)/$(BUILD).elf ; \
	then \
		echo File $(BUILDDIR)/$(BUILD).elf doesn\'t exist; \
		exit 1; \
	fi
	$(TOOLBIN)/elf -m $(BUILDDIR)/$(BUILD).elf

###########################################################################
#
# rebuild:
# Rebuild entirely.
#
rebuild:	clobber depend boot ramtst
	@echo 
	
###########################################################################
#
# dis:
# Generate a source/assembly file from the specified build.
#
dis:	buildcheck
	@if ! test -f $(BUILDDIR)/$(BUILD).elf ; \
	then \
		echo File $(BUILDDIR)/$(BUILD).elf doesn\'t exist; \
		exit 1; \
	fi
	$(OBJDUMP) --source --disassemble $(BUILDDIR)/$(BUILD).elf > \
		$(BUILDDIR)/$(BUILD).dis
	
###########################################################################
#
# newmon:
# Use the newmon tool to overwrite a running monitor.
# Allow the makefile to use "NEWMONBASE" as an override of the
# default "BOOTROMBASE" for specifying the base address at which
# newmon is to place the image.
#
newmon:	
	@if ! test -f $(BUILDDIR)/boot.bin ; \
	then \
		echo File $(BUILDDIR)/boot.bin doesn\'t exist; \
		exit 1; \
	fi
ifndef TARGET_IP
	@echo "Must specify TARGET_IP on command line (or environment)."
	@exit 1
endif
ifdef NEWMONBASE
	$(TOOLBIN)/newmon -u -B $(NEWMONBASE) $(TARGET_IP) $(BUILDDIR)/boot.bin 
else
ifndef BOOTROMBASE
	@echo "Must specify BOOTROMBASE or NEWMONBASE."
	@exit 1
endif
	$(TOOLBIN)/newmon -u -B $(BOOTROMBASE) $(TARGET_IP) $(BUILDDIR)/boot.bin 
endif

###########################################################################
#
# rundisable:
# Build the tfs.c file with -D TFS_RUN_DISABLE defined so that the
# image can be built with no ability to autoboot.
#
rundisable:
	$(CC) $(COMMON_CFLAGS) -D TFS_RUN_DISABLE $(CUSTOM_CFLAGS) \
		$(COMMON_INCLUDE) -o tfs.o $(COMDIR)/tfs.c

###########################################################################
#
# tar:
# Create a .tgz file that includes the current port & template source,
# portions of umon_main applicable to the port and umon_apps/demo_app.
# This assumes that umon_main and umon_ports are peer directories
# that reside under the top-most directory level called "umon".
#
tar: clean
	@rm -f *.tgz cscope.out tags
	@/bin/sh -c "cd $(UMONTOP)/host; make clean"
	@/bin/sh -c "cd $(UMONTOP)/../.. ; tar -cvzf umon_$(PLATFORM).tgz \
		umon/README umon/umon_main/README \
		umon/umon_main/host umon/umon_main/target/common \
		umon/umon_main/target/cpu/$(CPUTYPE) \
		umon/umon_main/target/dev umon/umon_main/target/make \
		umon/umon_main/target/flash/$(FLASHSUBDIR) \
		umon/umon_ports/template $(PORTDIR)/$(TGTDIR) \
		umon/umon_main/target/fs umon/umon_main/target/glib \
		umon/umon_main/target/zlib umon/umon_apps/demo \
		umon/umon_apps/user_manual" 
	mv $(UMONTOP)/../../umon_$(PLATFORM).tgz .

###########################################################################
#
# bdibuild:
# Build all the stuff needed to create the images (boot.bin & ramtst.elf)
# used for BDI2000 recovery.
#
bdibuild: clobber depend boot rundisable ramtst clean
	echo The ramtst target is built with "TFS run" disabled.

###########################################################################
#
# recovercopy:
# Primarily used by esutter to copy the recovery file set to the
# umon_recover directory.  This is usually done after a "make bdibuild"
# has done and tested.
#
recovercopy:
	@if ! test -f $(BUILDDIR)/boot.bin ; \
	then \
		echo File $(BUILDDIR)/boot.bin doesn\'t exist; \
		exit 1; \
	fi
	@if ! test -f $(BUILDDIR)/ramtst.elf ; \
	then \
		echo File $(BUILDDIR)/ramtst.elf doesn\'t exist; \
		exit 1; \
	fi
	@if ! test -f bdi2000.cfg ; \
	then \
		echo File bdi2000.cfg doesn\'t exist; \
		exit 1; \
	fi
	@if ! test -f bdi_hookup.jpg ; \
	then \
		echo File bdi_hookup.jpg doesn\'t exist; \
		exit 1; \
	fi
	@rm -rf ../../umon_recover/$(TGTDIR)
	@mkdir ../../umon_recover/$(TGTDIR)
	@mkdir ../../umon_recover/$(TGTDIR)/$(BUILDDIR)
	@cp $(BUILDDIR)/boot.bin ../../umon_recover/$(TGTDIR)/$(BUILDDIR)
	@cp $(BUILDDIR)/ramtst.elf ../../umon_recover/$(TGTDIR)/$(BUILDDIR)
	@cp bdi2000.cfg ../../umon_recover/$(TGTDIR)
	@cp bdi_hookup.jpg ../../umon_recover/$(TGTDIR)
	@echo Recovery file set for $(PLATFORM):
	@find ../../umon_recover/$(TGTDIR) -type f
	


###########################################################################
#
# dld:
# Use the ttftp tool to download the ramtst elf image into TFS.
#
dld:	strip	
ifndef TARGET_IP
	@echo "Must specify TARGET_IP on command line (or environment)."
	@exit 1
endif
	@if ! test -f $(BUILDDIR)/ramtst.elf ; \
	then \
		echo File $(BUILDDIR)/ramtst.elf doesn\'t exist; \
		exit 1; \
	fi
	$(TOOLBIN)/ttftp $(TARGET_IP) put $(BUILDDIR)/ramtst.elf ramtst,E

###########################################################################
#
# dldbin:
# Use the ttftp tool to download the ramtst binary image into RAM.
#
dldbin:
ifndef TARGET_IP
	@echo "Must specify TARGET_IP on command line (or environment)."
	@exit 1
endif
	@if ! test -f $(BUILDDIR)/ramtst.bin ; \
	then \
		echo File $(BUILDDIR)/ramtst.bin doesn\'t exist; \
		exit 1; \
	fi
	$(TOOLBIN)/ttftp $(TARGET_IP) put $(BUILDDIR)/ramtst.bin $(RAMTSTBASE)


###########################################################################
#
# help:
# Dump the common make targets and their purpose:
#
help:  help_local
	@echo The following generic make-targets are available:
	@echo "boot:     Build a bootrom-resident version of uMon"
	@echo "ramtst:   Build a ram-resident version of uMon for testing"
	@echo "clobber:  Remove all files built by this makefile."
	@echo "clean:    Remove all object files built by this makefile."
	@echo "ctags:    Build a tags file for use by most source editors."
	@echo "cscope:   Build a cscope.files file for use by cscope."
	@echo "libgcc:   Dump the libgcc used."
	@echo "depend:   Create a dependency list (file=depends) for the build."
	@echo "map:      Dump a map of the build using objdump."
	@echo "          This requires BUILD=xxxx to be specified on the"
	@echo "          make command line.  The value of xxxx will usually"
	@echo "          be 'boot', 'ram' or 'ramtst'."
	@echo "          Example: make BUILD=boot map"
	@echo "emap:     Dump a map of the build using the 'elf' tool."
	@echo "          See notes on 'map' above regarding BUILD."
	@echo "monsym:   Create the symbol table file (*.usym) used by"
	@echo "          MicroMonitor. See note in 'map' above regarding BUILD."
	@echo "dis:      Create a source/disassembly file (*.dis) of the image."
	@echo "          See note in 'map' above regarding BUILD."
	@echo "rebuild:  Concatenation of \"clobber, depend, boot and ramtst\""
	@echo "newmon:   Run through the steps needed to burn a new monitor"
	@echo "          Must specify TARGET_IP=a.b.c.d"
	@echo "dld:      Download the ramtst image to the target."
	@echo "          Must specify TARGET_IP=a.b.c.d"
	@echo "rundisable:"
	@echo "          Rebuild with the tfs run functionality disabled."
	@echo "bdibuild: Build the files used for BDI2000 disaster recovery."
	@echo
	@echo Build-specific outputs for $(PLATFORM) are suffixed as follows:
	@echo "  *.bin:    Raw binary image of build, suitable for transfer"
	@echo "            directly to memory."
	@echo "  *.elf:    ELF-formatted image."
	@echo "  *.dis:    Disassembly of uMon build."
	@echo "  *.gsym:   Symbol table formatted by gnu tools (nm)."
	@echo "  *.usym:   Symbol table formatted by uMon tools (monsym)."
	@echo and will be placed in the directory \"build_$(PLATFORM)\".
	@echo
