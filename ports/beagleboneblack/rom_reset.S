    .file "rom_reset.S"

/*
 * General notice:
 * This code is part of a boot-monitor package developed as a generic base
 * platform for embedded system designs.  As such, it is likely to be
 * distributed to various projects beyond the control of the original
 * author.  Please notify the author of any enhancements made or bugs found
 * so that all may benefit from the changes.  In addition, notification back
 * to the author will allow the new user to pick up changes that may have
 * been made by other users after this version of the code was distributed.
 *
 * Author:  Ed Sutter
 * email:   esutter@lucent.com
 * phone:   908-582-2351
 * 
 *
 * Modified for the CSB740 - OMAP3530 Single Board
 *
 * rom_reset.s:
  */

#include "warmstart.h"
#include "omap3530.h"
#include "config.h"

    /*
     * Have a separate stack for each processor mode.
     */

    /* define sizes for each mode's stack */
    .equ FiqStackSz, 4096
    .equ IrqStackSz, 4096
    .equ AbtStackSz, 4096
    .equ UndStackSz, 4096
    .equ SysStackSz, 4096

    /* declare the stacks */
    .extern MonStack
    .global FiqStack
    .global IrqStack
    .global AbtStack
    .global UndStack
    .global SysStack        
    .global raise
    .global cache_init
    
    /* allocate the stacks */
    .comm   FiqStack, FiqStackSz    /* for the FIQ mode */
    .comm   IrqStack, IrqStackSz    /* for the IRQ mode */
    .comm   AbtStack, AbtStackSz    /* for the Abort mode */
    .comm   UndStack, UndStackSz    /* for the Undef mode */
    .comm   SysStack, SysStackSz    /* for the System mode */
    /* User mode has the same stack as system mode. */

/*********************************************************************/
    
    .extern start

    .global reset
    .global coldstart
    .global lukewarmstart
    .global warmstart

    .text
        
    /*
     * Exception table at address 0
     */ 
reset:  
    b coldstart
    b undefined_instruction
    b software_interrupt
    b abort_prefetch
    b abort_data
    b not_assigned
    b interrupt_request
    b fast_interrupt_request

#include "etheraddr.S"
#include "moncomptr.S"
#include "alttfsdevtbl.S"

coldstart:
	ldr	pc, =coldstart_1		// jump to actual ROM location
	nop

coldstart_1:
	/* Make sure interrupts are off, and we're in supervisor mode...
	 */
	mrs	r0,cpsr				// Retreive current program status register
	bic	r0,r0,#0x1f			// Clear all mode bits.
	orr	r0,r0,#0xd3			// Set mode to supervisor, IRQ FIQ disabled.
	msr	cpsr,r0

//	bl cache_init

//----------------------------------------------------------
// Start of Cogent Setup for CSB740 OMAP3530
//----------------------------------------------------------

init_pbias:
	ldr r2, =0x00000000			// set bias for sdio1 
	ldr r1, =0x48002520
	str r2, [r1]

	bl	delay_200

	ldr r2, =0x00000606			// set bias for sdio1
	ldr r1, =0x48002520
	str r2, [r1]

	bl	delay_200
init_clocks:
	ldr r2, =0x00000037			// Enable DPLL1 in lock mode
	ldr r1, =0x48004904
	str r2, [r1]

	bl	delay_200

	ldr r2, =0x000A7115			// Set DPLL1 (MPU) M = 625, (N +1)= 21 + 1, MPU_CLK = ~545MHz
	ldr r1, =0x48004940
	str r2, [r1]

	bl	delay_200

	ldr r2, =0x099F1700			// Set DPLL3 (CORE) M = 415, (N +1)= 23 + 1, CORE_CLK = ~332MHz
	ldr r1, =0x48004D40
	str r2, [r1]

	bl	delay_200

	//ldr r2, =0x00000080			// Enable SYS_CLKOUT2 for debug purposes
	//ldr r1, =0x48004D70
	//str r2, [r1]

	//bl	delay_200

	ldr r2, =0x43fffe00			// Turn on all available module clocks
	ldr r1, =0x48004a00
	str r2, [r1]

	bl	delay_200

	ldr r2, =0x7ffffedb			// Turn on all available peripheral clocks
	ldr r1, =0x48004a10
	str r2, [r1]

	bl	delay_200

	ldr r2, =0x00006000			// enable auto clock for UART1 and UART2
	ldr r1, =0x48004a30
	str r2, [r1]

	bl	delay_200
	
	ldr r2, =0x00000028			// enable WDT2 and GPIO 1 functional clock
	ldr r1, =0x48004c00
	str r2, [r1]

	bl	delay_200
	
	ldr r2, =0x0000002c			// enable WDT2, GPIO 1 interface and 32Ksync (for Linux) clock
	ldr r1, =0x48004c10
	str r2, [r1]

	bl	delay_200
	
	ldr r2, =0x0003E000			// enable GPIO 2-6 functional clocks
	ldr r1, =0x48005000
	str r2, [r1]

	bl	delay_200
	
	ldr r2, =0x0003E000			// enable GPIO 2-6 interface clocks
	ldr r1, =0x48005010
	str r2, [r1]

	bl	delay_200
	
	ldr r2, =0x00000003			// enable DSS1_ALWON_FCLK
	ldr r1, =0x48004e00
	str r2, [r1]

	bl	delay_200
	
	ldr r2, =0x00000001			// enable DSS interface clock
	ldr r1, =0x48004e10
	str r2, [r1]

	bl	delay_200
	
	ldr r2, =0x0000100A			// Set CLKSEL_DSS1 to divide by 1
	ldr r1, =0x48004e40
	str r2, [r1]

	bl	delay_200
	
init_ddr:
    ldr r2, =0x0000001A			// reset DDR 
    ldr r1, =0x6D000010
    str r2, [r1]

    ldr r1, =0x6D000014			// SDRC_SYSSTATUS
wait_reset:
    ldr r2, [r1]
    tst r2, #1				// test RESETDONE
    beq wait_reset

    ldr r2, =0x00000018			// release DDR reset
    ldr r1, =0x6D000010
    str r2, [r1]

	bl	delay_200

    ldr r2, =0x00000100			// 32-bit SDRAM on data lane [31:0] - CS0
    ldr r1, =0x6D000044
    str r2, [r1]

	bl	delay_200

    ldr r2, =0x02584099			// SDRC_MCFG0 register
    ldr r1, =0x6D000080
    str r2, [r1]

	bl	delay_200

    ldr r2, =0x00054601			// SDRC_RFR_CTRL0 register
    ldr r1, =0x6D0000a4
    str r2, [r1]

	bl	delay_200

    ldr r2, =0xA29DB4C6			// SDRC_ACTIM_CTRLA0 register
    ldr r1, =0x6D00009c
    str r2, [r1]

	bl	delay_200

    ldr r2, =0x00012214			// SDRC_ACTIM_CTRLB0 register
    ldr r1, =0x6D0000A0
    str r2, [r1]

	bl	delay_200

    ldr r2, =0x00000081			// Disble Power Down of CKE due to 1 CKE on combo part
    ldr r1, =0x6D000070
    str r2, [r1]

	bl	delay_200

    ldr r2, =0x00000000			// NOP command 
    ldr r1, =0x6D0000A8
    str r2, [r1]

	bl	delay_200

    ldr r2, =0x00000001			// Precharge command  
    ldr r1, =0x6D0000A8
    str r2, [r1]

	bl	delay_200

    ldr r2, =0x00000002			// Auto-refresh command  
    ldr r1, =0x6D0000A8
    str r2, [r1]

	bl	delay_200

    ldr r2, =0x00000002			// Auto-refresh command  
    ldr r1, =0x6D0000A8
    str r2, [r1]

	bl	delay_200

    ldr r2, =0x00000032			// SDRC MR0 register Burst length=4  
    ldr r1, =0x6D000084
    str r2, [r1]

	bl	delay_200

    ldr r2, =0x0000000A			// SDRC DLLA control register  
    ldr r1, =0x6D000060
    str r2, [r1]

	bl	delay_200

/********************************************************************/

midstart:
    ldr r0, =INITIALIZE
    
    /* fall-through to 'lukewarmstart' */

/********************************************************************/
    
lukewarmstart:  
    /* Save the argument to r11 */
    mov r11, r0

    /*
     * *** DO NOT TOUCH R11 ***
     */ 

    /*
     * Set-up the stack-pointers for all operating modes
     */

    /* FIQ mode */
    mrs r0, cpsr                /* move CPSR to r0 */
    bic r0, r0, #0x1f           /* clear all mode bits */
    orr r0, r0, #0x11           /* set FIQ mode bits */
    msr CPSR_c, r0              /* move back to CPSR */
    ldr sp, =(FiqStack + FiqStackSz - 4)    /* initialize the stack ptr */
    /* IRQ mode */
    mrs r0, cpsr                /* move CPSR to r0 */
    bic r0, r0, #0x1f           /* clear all mode bits */
    orr r0, r0, #0x12           /* set IRQ mode bits */
    msr CPSR_c, r0              /* move back to CPSR */
    ldr sp, =(IrqStack + IrqStackSz - 4)    /* initialize the stack ptr */
    /* Abort mode */
    mrs r0, cpsr                /* move CPSR to r0 */
    bic r0, r0, #0x1f           /* clear all mode bits */
    orr r0, r0, #0x17           /* set Abort mode bits */
    msr CPSR_c, r0              /* move back to CPSR */
    ldr sp, =(AbtStack + AbtStackSz - 4)    /* initialize the stack ptr */
    /* Undef mode */
    mrs r0, cpsr                /* move CPSR to r0 */
    bic r0, r0, #0x1f           /* clear all mode bits */
    orr r0, r0, #0x1b           /* set Undef mode bits */
    msr CPSR_c, r0              /* move back to CPSR */
    ldr sp, =(UndStack + UndStackSz - 4)    /* initialize the stack ptr */
    /* System mode */
    mrs r0, cpsr                /* move CPSR to r0 */
    bic r0, r0, #0x1f           /* clear all mode bits */
    orr r0, r0, #0x1f           /* set System mode bits */
    msr CPSR_c, r0              /* move back to CPSR */
    ldr sp, =(SysStack + SysStackSz - 4)    /* initialize the stack ptr */
    /* 'warmstart' will take us back to SVC mode
       stack for SVC mode will also be setup in warmstart */

    mov r0, r11     /* get argument back from r11 */
    b   warmstart

    
/********************************************************************/
    
warmstart:
    /* Save the argument to r11 */
    mov r11, r0

    /*
     * *** DO NOT TOUCH R11 ***
     */ 


    /* Change (back) to SVC mode */
    mrs r0, cpsr                /* move CPSR to r0 */
    bic r0, r0, #0x1f           /* clear all mode bits */
    orr r0, r0, #0x13           /* set System mode bits */
    msr CPSR_c, r0              /* move back to CPSR */
    /* Reset the stack pointer for the SVC mode (our current mode) */
    ldr sp, =(MonStack + MONSTACKSIZE - 4)

    /*
     * Restore argument which was saved to r11 and jump to
     * the C function start().
     */ 
    
    mov r0, r11
jump_to_c:
    bl start

    /* the C code should never return */
    b reset

.align 4


/*********************************************************************
 * simple delay loop
 */
delay_200:
	ldr		r3, =200			/* loop count */
delay_loop:
	subs	r3,r3,#1
	bne		delay_loop
	nop

	mov		pc, lr

raise:	mov		pc, lr  /* to make linker happy */

/*********************************************************************
 * Cache initialization:
 * Turn everything down and invalidate...
 */
cache_init:
	/* Make sure caches are turned down...
	 */
 	mrc	p15, 0, r3, cr1, cr0, 0		// turn off I/D-cache
 	bic	r3, r3, #4096				// I
 	bic	r3, r3, #4					// D
 	mcr	p15, 0, r3, cr1, cr0, 0
	
 	mov	r0, #0
//	mcr	p15, 0, r0, cr7, cr7, 0  // arm_cache_invalidate
	mcr	p15, 0, r0, cr7, cr6, 0  // arm_dcache_invalidate
	mcr	p15, 0, r0, cr7, cr5, 0  // arm_icache_invalidate

 	mrc	p15, 0, r0, cr1, cr0, 1  // l2cache_disable
 	bic	r0, r0, #2
 	mcr	p15, 0, r0, cr1, cr0, 1

 	mov	r0, #1
	mrc	p15, 1, r0, cr0, cr0, 1  // emu_ext_boot_l2_inv
	mov		pc, lr

