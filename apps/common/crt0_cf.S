#include "cfg.h"
    
    .extern	AppStack
	.extern	Cstart
    .global start

    .text

start:
    move.l	#AppStack+(APPSTACKSIZE-32),%sp
    jsr		Cstart
