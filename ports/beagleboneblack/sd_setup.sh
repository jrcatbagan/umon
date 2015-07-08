#!/usr/bin/bash
#
# This script automates the process of setting up an SD card for
# either "raw" or FAT mode booting for the Beaglebone Black.

if [ $# -ne 2 ]; then
	echo -e  "Please specify both the boot mode and the SD card to set up\n"
	echo -e "Usage: ./sd_setup.sh <boot mode> /dev/<device>\n"
	echo -e "where <boot mode> is either RAW or FAT and <device> is the SD card"
	exit 1
fi

BOOTMODE=$1
SDDEV=$2

case $1 in
RAW)
	if [ ! -e ./build_BEAGLEBONEBLACK/rawboot.bin ]; then
		echo -e "rawboot.bin does not exist in ./build_BEAGLEBONEBLACK\n"
		echo -e "Please build uMon before proceeding"
		exit 1
	fi

	# Clear all offsets where a vaild uMon image for "raw" mode booting can exist
	dd if=/dev/zero of=${SDDEV} bs=1M count=1

	# Get the size of the uMon image to transfer into the SD card.
	UMON_IMG_SIZE=`wc --bytes ./build_BEAGLEBONEBLACK/rawboot.bin | cut -f 1 -d ' '`

	# Store the uMon image at offset 0x00000 by default
	dd if=./build_BEAGLEBONEBLACK/rawboot.bin of=${SDDEV} bs=1 count=${UMON_IMG_SIZE}
	;;
FAT)
	if [ ! -e ./build_BEAGLEBONEBLACK/MLO ]; then
		echo -e "MLO does not exist in ./build_BEAGLEBONEBLACK\n"
		echo -e "Please build uMon before proceeding"
		exit 1
	fi

	# Clear the partition table at the base of the SD card
	dd if=/dev/zero of=${SDDEV} bs=1M count=1

	# Create the FAT16 primary partition and mark it bootable
	echo -e "n\np\n1\n\n+3M\nt\n6\na\nw\n" | fdisk ${SDDEV}

	# Wait for some time to allow the partition to register under /dev
	sleep 1

	mkfs.fat -v -f 2 -F 16 -M 0xF8 -s 1 -S 512 ${SDDEV}1

	mkdir mnt
	mount ${SDDEV}1 mnt
	cp ./build_BEAGLEBONEBLACK/MLO mnt
	umount mnt
	rm -rf mnt
	;;
*)
	echo -e "Invalid boot mode specified.  Valid boot modes are RAW/FAT."
	exit 1
	;;
esac

