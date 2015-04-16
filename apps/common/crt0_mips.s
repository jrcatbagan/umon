#include "cfg.h"

#define sp	$29
#define k0	$26
#define KSEG_MSK	0xE0000000
#define K0BASE		0x80000000
#define KSEG1A(reg) \
	and reg, ~KSEG_MSK; \
	or reg, K1BASE;
	
	.extern AppStack
	.global start

	.text
	.set noreorder

start:
	la		sp, AppStack
	addiu	sp, APPSTACKSIZE
	addiu 	sp, -4*4

goToC:
	la		k0,Cstart
	j		k0
	nop
