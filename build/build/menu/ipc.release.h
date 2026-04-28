
############################################################
# choose release files.

menu "Rlease Files"
	depends on IPC_FLASH_NAND
	
config IPC_RLF_UPDATE
	bool "Update file"
	
config IPC_RLF_FLASH
	bool "Flash file"

endmenu


# <choose autolens/> ------------------------------------#

############################################################
