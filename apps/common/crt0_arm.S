#include "cfg.h"

    .extern AppStack
	.extern Cstart
	.global	start

    /* Set stack pointer to end of AppStack and jump to Cstart:
	 */
start:
    ldr sp, =(AppStack + APPSTACKSIZE - 4)

jump_to_c:
    bl Cstart
