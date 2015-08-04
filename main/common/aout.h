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
 * aout.h:
 *
 *  Header file for the AOUT file format used by TFS.
 *
 * Original author:     Ed Sutter (ed.sutter@alcatel-lucent.com)
 *
 */
#ifndef _AOUT_H_
#define _AOUT_H_

struct exec {
    unsigned short  a_mid;
    unsigned short  a_magic;
    unsigned long   a_text;     /* Size of text segment in bytes */
    unsigned long   a_data;     /* Size of data segment in bytes */
    unsigned long   a_bss;      /* Size of bss segment in bytes */
    unsigned long   a_syms;     /* Size of symbol table in bytes */
    unsigned long   a_entry;    /* Entry point address */
    unsigned long   a_trsize;   /* Size of text relocation table */
    unsigned long   a_drsize;   /* Size of data relocation table */
};

struct relocation_info {
    int     r_address;
    ulong       r_info;
};

/* Fields within r_info: */
#define SYMNUM_MSK  0xffffff00
#define PCREL_MSK   0x00000080
#define LENGTH_MSK  0x00000060
#define EXTERN_MSK  0x00000010
#endif
