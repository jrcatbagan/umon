    .file "ram_reset.s"

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
 * ram_reset.s:
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
    .global ipaddr
    .global etheraddr
    .global moncomptr

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

/*********************************************************************/

    /*
     * At the end of the reset sequence, MMU, Icache, Dcache,
     * and write buffer are all disabled.
     * Also IRQs and FIQs are disabled in the processor's CPSR
     * The operating mode is SVC (supervisory mode), and the
     * PC is vectored at 0x00000000. A branch in 0x00000000
     * brings us directly here.
	 *
	 */

coldstart:
//	ldr r0, =0x2001					/* allow access to all coprocessors */
//	mcr 	p15,0,r0,c15,c1,0
//	nop
//	nop
//	nop

//	ldr 	r0, =0x00000078
//	mcr 	p15,0,r0,c1,c0,0		/* Disable MMU, caches, write buffer */
//	nop
//	nop
//	nop

//	ldr 	r0, =0x00000000
//	mcr     p15,0,r0,c8,c7,0   		/* flush TLB's */
//	mcr     p15,0,r0,c7,c7,0   		/* flush Caches */
//	mcr		p15,0,r0,c7,c10,4		/* Flush Write Buffer */
//	nop
//	nop
//	nop

//	mvn    r0, #0                   /* grant manager access to all domains */
//	mcr    p15,0,r0,c3,c0,0  

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

