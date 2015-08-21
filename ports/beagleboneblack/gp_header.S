	.file "gp_header.S"

/*
 * Copyright (c) 2015 Jarielle Catbagan <jcatbagan93@gmail.com>
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 *
 * This GP header is prepended to uMon images when booting from non-XIP devices (e.g. SD).
 * Refer to the AM335x TRM, Section 26.1.9 for more information.
 */

	.word BIN_SIZE + 8
	.word 0x402f0400
