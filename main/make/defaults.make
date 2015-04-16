############################################################################
#
# defaults.make:
# Miscellaneous defaults that can be overridden in the makefile.
#
# LEDIT:
# Line editor mode (vt100 or vi)...
LEDIT		= ledit_vt100.c

# TFSCLEAN:
# Powersafe defrag (tfsclean1) or non-powersafe defrag (tfsclean2)...
TFSCLEAN	= tfsclean1.c

# FLASHDIR:
# For systems that use the newer style flash driver, use
# $(BASE)/flash/devices, or for the older drivers, refer
# to the flash/boards directory.
FLASHDIR	= $(BASE)/flash/devices

# DHCP:
# For standard DHCP, use dhcp_00.c, else refer to other dhcp_XX.c file
# for options.
DHCP		= dhcp_00.c
