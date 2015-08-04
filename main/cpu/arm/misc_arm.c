/**************************************************************************
 *
 * Copyright (c) 2013 Alcatel-Lucent
 *
 * Alcatel Lucent licenses this file to You under the Apache License,
 * Version 2.0 (the "License"); you may not use this file except in
 * compliance with the License.  A copy of the License is contained the
 * file LICENSE at the top level of this repository.
 * You may also obtain a copy of the License at:
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 **************************************************************************
 *
 * misc_arm.c
 *
 * Original author:     Ed Sutter (ed.sutter@alcatel-lucent.com)
 *
 */

/*
 * Set the Current Program Status Register.
 * old way assumed argument was in R0:
 *  asm("   msr CPSR_c, r0");
 */
void
putpsr(unsigned long psr)
{
    volatile unsigned register reg;

    reg = psr;
    asm volatile("msr CPSR_c, %0" : "=r"(reg));
}

/*
 * Return the Current Program Status Register.
 * old way assumed return in R0:
 *  asm("   mrs r0, CPSR");
 */
unsigned long
getpsr(void)
{
    volatile unsigned register reg;
    asm volatile("mrs %0, CPSR" : "=r"(reg));
    return(reg);
}

/* getsp():
 * Return the current stack pointer.
 *  oldway: asm("   mov r0, r13");
 */
unsigned long
getsp(void)
{
    volatile unsigned register reg;
    asm volatile("mov %0, r13" : "=r"(reg));
    return(reg);
}
