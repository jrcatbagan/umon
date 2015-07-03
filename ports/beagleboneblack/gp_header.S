	.file "gp_header.S"

/* This GP header is prepended to uMon images when booting from non-XIP devices (e.g. SD).
 * Refer to the AM335x TRM, Section 26.1.9 for more information.
 */
	.word BIN_SIZE + 8
	.word 0x402f0400
