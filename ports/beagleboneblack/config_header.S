	.file "config_header.S"

/*
 * Copyright (c) 2015 Jarielle Catbagan <jcatbagan93@gmail.com>
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 *
 * This is the Configuration Header TOC Structure that must be prepended to a GP Header followed
 * by the uMon image in order to boot from non-XIP devices such as an SD card using "raw" mode.
 * Please refer to the AM335x TRM, Section 26.1.7.5.5.1 "Configuration Header" for more information.
 */

	/* Start */
	.word 0x000000a0
	/* Size */
	.word 0x00000050
	/* Reserved */
	.word 0x00000000
	.word 0x00000000
	.word 0x00000000
	/* Filename */
	.word 0x45534843
	.word 0x4e495454
	.word 0x00005347
	/* Closing Item */
	.word 0xffffffff
	.word 0xffffffff
	.word 0xffffffff
	.word 0xffffffff
	.word 0xffffffff
	.word 0xffffffff
	.word 0xffffffff
	.word 0xffffffff

	/* Gap between Configuration Header TOC Structure and Configuration Header Settings */
	.rept 24
	.word 0x00000000
	.endr

	/* Section Key */
	.word 0xc0c0c0c1
	/* Valid */
	.byte 0x00
	/* Version */
	.byte 0x01
	/* Reserved */
	.rept 86
	.word 0x00000000
	.endr
	.byte 0x00
	.byte 0x00
