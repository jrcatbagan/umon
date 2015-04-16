#include "cfg.h"

    .extern AppStack
	.extern Cstart
	.global	start

    /* Set stack pointer to end of AppStack and jump to Cstart:
	 */
start:
	addi r1, r0, AppStack
	addi r1, r1, (APPSTACKSIZE-16)

jump_to_c:
    brai Cstart
