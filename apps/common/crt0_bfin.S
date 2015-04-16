#include "cfg.h"
    
    .extern	_AppStack
	.extern	_Cstart
    .global start

    .text

start:
	/* Clear all DAG registers:
	 */
	R0 = 0
	I0 = R0
	I1 = R0
	I2 = R0
	I3 = R0
	L0 = R0
	L1 = R0
	L2 = R0
	L3 = R0
	B0 = R0
	B1 = R0
	B2 = R0
	B3 = R0
	M0 = R0
	M1 = R0
	M2 = R0
	M3 = R0
	LC0 = R0;
	LC1 = R0;

    sp.h = _AppStack+(APPSTACKSIZE-32)
    sp.l = _AppStack+(APPSTACKSIZE-32)
    jump	_Cstart
