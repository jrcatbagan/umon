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
 * pci.h:
 *
 * GENERAL PCI Definitions.
 * The majority of this is extracted from the PCI Local Bus Specification
 * Revision 2.2.
 *
 * Original author:     Ed Sutter (ed.sutter@alcatel-lucent.com)
 */

/* Layout of configuration address register:
 * See section 3.2.2.3.2 (Software Generation of Configuration
 * Transactions, pg 32).
 */
#ifndef _PCI_H_
#define _PCI_H_

#define PCICFG_ENABLE_BIT		0x80000000
#define PCICFG_BUSNO_MASK		0x000000ff
#define PCICFG_BUSNO_SHIFT		16
#define PCICFG_DEVNO_MASK		0x0000001f
#define PCICFG_DEVNO_SHIFT		11
#define PCICFG_FNCNO_MASK		0x00000007
#define PCICFG_FNCNO_SHIFT		8
#define PCICFG_REGNO_MASK		0x0000003f
#define PCICFG_REGNO_SHIFT		2

/* Command register layout (as it would be in the 32-bit register)
 * See section 6.2.2 (Device Control, pg 193).
 */
#define FAST_BKTOBK_ENABLE		0x00000200
#define SERR_ENABLE				0x00000100
#define STEPPING_CTRL			0x00000080
#define PARITY_ERROR_RESP		0x00000040
#define VGA_PALETTE_SNOOP		0x00000020
#define MEM_WRITE_INV_ENABLE	0x00000010
#define SPECIAL_CYCLES			0x00000008
#define BUS_MASTER				0x00000004
#define MEMORY_SPACE			0x00000002
#define IO_SPACE				0x00000001

/* Status register layout (as it would be in the 32-bit register)
 * See section 6.2.3 (Device Status, pg 196).
 */
#define DETECTED_PARITY_ERR		0x80000000
#define SIGNALED_SYSTEM_ERR		0x40000000
#define RECVD_MASTER_ABORT		0x20000000
#define RECVD_TARGET_ABORT		0x10000000
#define SIGNALED_TARGET_ABORT	0x08000000
#define DEV_SEL_TIMING_MASK		0x06000000
#define DEV_SEL_TIMING_FAST		0x00000000
#define DEV_SEL_TIMING_MEDIUM	0x02000000
#define DEV_SEL_TIMING_SLOW		0x04000000
#define MASTER_DATA_PARITY_ERR	0x01000000
#define FAST_BKTOBK_CAPABLE		0x00800000
#define CAPABLE_66MHZ			0x00200000
#define CAPABILITIES_LIST		0x00100000

/* BaseAddressMapping
 * See section 6.2.5.1 (Address Maps, pg 202).
 */
#define IMPLEMENTED				0x80000000
#define BASEADDRESS_IO			0x00000001
#define BASEADDRESS_MEM			0x00000000
#define PREFETCHABLE			0x00000008
#define TYPE_MASK				0x00000006
#define TYPE_32					0x00000000
#define TYPE_64					0x00000004

/* HeaderType definitions, as they  would be in the 32-bit register.
 * See section 6.2.1, pg 192.
 */
#define HDR_MASK				0x00ff0000
#define HDR_STANDARD			0x00000000
#define HDR_PCI2PCI				0x00010000
#define HDR_CARDBUS				0x00020000
#define HDR_MULTIFUNC			0x00800000

/* BIST bits, as they would be in the 32-bit register.
 * See section 6.2.4 pg 199.
 */
#define BIST_CAPABLE			0x80000000
#define BIST_START				0x40000000
#define BIST_COMPCODE_MASK		0x0f000000

/*-----------------------------------------------------------------------------
 * Mappings for PCI memory and IO spaces
 */

/* Macros for getting an individual device or function from a combined
 * device/function number
 */
#define PCI_MAKE_DEV_FUNC(_dev_, _func_) 	(((_dev_)<<3)|(_func_))
#define PCI_GET_DEV(_dev_func_) 			((_dev_func_ >> 3) & 0x1f)
#define PCI_GET_FUNC(_dev_func_) 			(_dev_func_ & 0x7)

/* Macros for BUS, FUNC and DEV in PCI_CFG_ADD
 */
#define PCI_CFG_BUS(_x_)	((_x_ & 0xff) << 16)	/* Bus Number 	 	*/
#define PCI_CFG_DEV(_x_) 	((_x_ & 0xf8) << 11)	/* Device Number	*/
#define PCI_CFG_FUNC(_x_)	((_x_ & 0x07) << 8) 	/* Function Number	*/
#define PCI_CFG_REG(_x_)	((_x_ & 0xfc) << 0)		/* Register Number	*/

/*-----------------------------------------------------------------------------
 * Configuration header offsets
 *-----------------------------------------------------------------------------
 */
#define	PCI_CFG_VEN_ID_REG 				0x0
#define	PCI_CFG_DEV_ID_REG 				0x2
#define	PCI_CFG_CMD_REG 				0x4
#define	PCI_CFG_STAT_REG 				0x6
#define	PCI_CFG_REV_REG	 				0x8
#define	PCI_CFG_IF_REG 					0x9
#define	PCI_CFG_SUBCLASS_REG 			0xa
#define	PCI_CFG_CLASS_REG 				0xb
#define	PCI_CFG_HDR_TYPE_REG 			0xe
#define	PCI_CFG_BIST_REG 				0xf
#define	PCI_CFG_BAR0_REG 				0x10
#define	PCI_CFG_BAR1_REG 				0x14
#define	PCI_CFG_BAR2_REG 				0x18
#define	PCI_CFG_BAR3_REG 				0x1c
#define	PCI_CFG_BAR4_REG 				0x20
#define	PCI_CFG_BAR5_REG 				0x24
#define	PCI_CFG_CIS_REG	 				0x28	/* Cardbus only */
#define PCI_CFG_SUB_VEN_ID_REG			0x2c
#define PCI_CFG_SUB_DEV_ID_REG			0x2e
#define PCI_CFG_ROM_BASE_REG			0x30
#define PCI_CFG_RES0_REG				0x34
#define PCI_CFG_RES1_REG				0x38
#define PCI_CFG_INT_REG					0x3c
#define PCI_CFG_CACHE_REG				0x3d
#define PCI_CFG_GNT_REG					0x3e
#define PCI_CFG_LAT_REG					0x3f

/* bridge devices only
 */
#define	PCI_CFG_BAR0_REG 				0x10
#define	PCI_CFG_BAR1_REG 				0x14
#define	PCI_CFG_PRI_BUS_REG				0x18
#define	PCI_CFG_SEC_BUS_REG				0x19
#define	PCI_CFG_SUB_BUS_REG				0x1a
#define	PCI_CFG_SEC_LAT_REG				0x1b
#define	PCI_CFG_IO_BASE_LO_REG			0x1c
#define	PCI_CFG_IO_LIMIT_LO_REG			0x1d
#define	PCI_CFG_MEM_BASE_REG			0x20
#define	PCI_CFG_MEM_LIMIT_REG			0x22
#define	PCI_CFG_PREFETCH_BASE_REG		0x24
#define	PCI_CFG_PREFETCH_LIMIT_REG		0x26
#define	PCI_CFG_IO_BASE_HI_REG			0x30
#define	PCI_CFG_IO_LIMIT_HI_REG			0x32

/* CLASS Codes
 */
#define PCI_CLASS_OLD					0x00
#define PCI_CLASS_MASS					0x01
#define PCI_CLASS_NET					0x02
#define PCI_CLASS_DISPLAY				0x03
#define PCI_CLASS_MULTIMEDIA			0x04
#define PCI_CLASS_MEMORY				0x05
#define PCI_CLASS_BRIDGE				0x06
#define PCI_CLASS_SIMPLE_COMM			0x07
#define PCI_CLASS_BASE_PERIPH			0x08
#define PCI_CLASS_INPUT					0x09
#define PCI_CLASS_DOCK					0x0a
#define PCI_CLASS_CPU					0x0b
#define PCI_CLASS_SERIAL_BUS			0x0c
#define PCI_CLASS_UNKNOWN				0xff

/* BRIDGE SUBCLASS Codes
 */
#define PCI_SUBCLASS_BRIDGE_PCI_PCI		0x04

/* BAR types
 */
#define PCI_MEM_BAR_TYPE_32BIT			0x00
#define PCI_MEM_BAR_TYPE_1M				0x01
#define PCI_MEM_BAR_TYPE_64BIT			0x02

/* PCI Command Register bit defines
 */
#define PCI_CFG_CMD_IO              	0x0001
#define PCI_CFG_CMD_MEM		        	0x0002
#define PCI_CFG_CMD_MASTER          	0x0004
#define PCI_CFG_CMD_SPECIAL         	0x0008
#define PCI_CFG_CMD_INVALIDATE      	0x0010
#define PCI_CFG_CMD_VGA_SNOOP       	0x0020
#define PCI_CFG_CMD_PARITY          	0x0040
#define PCI_CFG_CMD_WAIT            	0x0080
#define PCI_CFG_CMD_SERR            	0x0100
#define PCI_CFG_CMD_FAST_BACK       	0x0200

/* PCI Status Register bit defines
 */
#define PCI_CFG_STAT_PERR			0x8000	/* 1 = MPC8245 detected an */
											/* addr or data parity error */
#define PCI_CFG_STAT_SERR			0x4000	/* 1 = MPC8245 asserted SERR.*/
#define PCI_CFG_STAT_MST_ABRT		0x2000	/* 1 = MPC8245 asserted */
											/* master-abort. */
#define PCI_CFG_STAT_TGT_ABRT_MST	0x1000	/* 1 = MPC8245 received a */
											/* target-abort while acting */
											/* as a PCI master. */
#define PCI_CFG_STAT_TGT_ABRT_TGT	0x0800	/* 1 = MPC8245 asserted a */
											/* target-abort while acting */
											/* as a PCI target. */
#define PCI_CFG_STAT_PERR_DAT		0x0100	/* 1 = MPC8245 detected a */
											/* data parity error while */
											/* acting as a PCI master. */

/*--------------------------------------------------------------------------
 * Error messages from pci_probe_bus
 */
#define PCI_DEV_MAX_ERR  		0x01 	/* Ran out of space in */
										/* static structure, is this */
										/* really an error?	*/
#define PCI_IO_BAR_MAX_ERR		0x02	/* IO Dev asked for too much space */
#define PCI_IO_SPACE_ERR		0x03	/* We ran out of IO space to give */
#define PCI_MEM_BAR_TYPE_ERR	0x04	/* Unsupported BAR type */
#define PCI_MEM_BAR_MAX_ERR		0x05	/* MEM Dev asked for too much space	*/
#define PCI_MEM_SPACE_ERR		0x06	/* We ran out of MEM space to give */
#define PCI_BRIDGE_TYPE_ERR		0x07	/* Unsupported PCI Bridge type */

/* Probe limits
 */
#define PCI_IO_BAR_MAX			0x00100000	/* 1Mbyte IO Space per device */
#define PCI_MEM_BAR_MAX			0x01000000	/* 16Mbyte MEM Space per device */
#define PCI_DEV_MAX				31			/* 32 devices total */

/*--------------------------------------------------------------------------
 * Structure that gets filled in by pci_bus_probe
 */
typedef struct              /* PCI device data */
{
    unsigned short ven_id;      	/* vendor ID */
    unsigned short dev_id;          /* device ID */
    unsigned char class;          	/* device Class */
    unsigned char subclass;			/* device Subclass */
    unsigned long bar_size[6];		/* Memory size for each base address, */
									/* 0 for unused BAR's */
    unsigned long bar_map[6];		/* Physical address mapped - */
									/* 0 for unused BAR's */
	unsigned long func;				/* The function number - usually 0 */
	unsigned long dev;				/* The device number */
	unsigned long bus;				/* The bus this device resides on. */
									/* Non-0 means it's across a bridge */
} pci_device_structure;


/* See text of section 6.1 (Configuration Space Organization, pg 190) for an
 * explanation of why 0xffff is used to indicate "no device" in a
 * particular PCI slot.  For all configuration space of a slot that is
 * unoccupied, the PCI bridge device must return all ones.
 *
 * (Note that read accesses to unused registers within a valid PCI device
 * should return 0).
 */
#define NO_DEVICE				0xffffffff


/* Commands used by pciCtrl():
 */
#define PCICTRL_INIT	1

/* Functions in pci.c:
 */
extern int pciVerbose;
extern unsigned long pciCfgAddress(int busno,int devno,int fncno,int regno);
extern char *pciBaseClass(unsigned long classcode);

/****************************************************************************
 *
 * Functions needed by pci.c, that must be defined in the target
 * specific source code:
 */
/* pciCtrl()
 * Parameters:
 *	int interface-
 *		This parameter supports the case where the target hardware has more
 *		than one pci controller.  The interface number would correspond to
 *		one of potentially several different controllers.
 *	int cmd-
 *		Command to be carried out by the control function.
 *	ulong arg1-
 *		First command-specific argument.
 *	ulong arg2-
 *		Second command-specific argument.
 */
extern int pciCtrl(int interface,int cmd,unsigned long arg1,unsigned long arg2);

/* pciCfgWrite()
 * Parameters:
 *	int interface-
 *		Refer to description in pciCtrl() above.
 *	int bus-
 *		Bus number.
 *	int dev-
 *		Device number on bus.
 *	int func-
 *		Function number on device.
 *	int reg-
 *		Register number to be written.  Each reg number represents one 32-bit
 *		chunk of space in the configuration; hence, reg0 is offset 0, reg1 is
 *		offset 4, reg2 is offset 8, etc...
 *	ulong val-
 *		Value to be written into the specified config location.
 *	Return:
 *	int
 *		Return negative if failure, else 0.
 */
extern int pciCfgWrite(int interface,int bus,int dev,int func,int reg,
	unsigned long val);

/* pciCfgRead()
 * Parameters:
 *	int interface-
 *		Refer to description in pciCtrl() above.
 *	int bus-
 *		Bus number.
 *	int dev-
 *		Device number on bus.
 *	int func-
 *		Function number on device.
 *	int reg-
 *		Register number to be written.  Each reg number represents one 32-bit
 *		chunk of space in the configuration; hence, reg0 is offset 0, reg1 is
 *		offset 4, reg2 is offset 8, etc...
 *	Return:
 *	ulong
 *		Return the value stored in the specified config location.
 */
extern unsigned long pciCfgRead(int interface,int bus,int dev,
	int func,int reg);

/* pciShow()
 * If appropriate, dump text to the user that verbosely describes the
 * devices on the PCI bus.  If the devices on the bus are not fixed, then
 * this function should return a message indicating that.
 *
 * Parameters:
 *	int interface-
 *		Refer to description in pciCtrl() above.
 */
extern void pciShow(int interface);

#endif
