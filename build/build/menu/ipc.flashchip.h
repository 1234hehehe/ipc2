
choice IPC_FLASH_
    prompt "Flash Model"

config IPC_FLASH_NOR8M_
	bool "NOR 8MB"
	depends on IPC_FLASH_NOR8M
	
config IPC_FLASH_NOR16M_
	bool "NOR 16MB"
	depends on IPC_FLASH_NOR16M
	
config IPC_FLASH_NANDxxxM_BS128K_PS2K_SS64
	depends on IPC_FLASH_NAND 
	bool "Total:128M/256M/512M,Block:128K,Page:2K,Spare:64."
comment "FS35ND01G,FS35ND02G,FS35ND04G,MX35LF1GE4AB"
	depends on IPC_FLASH_NAND

config IPC_FLASH_NANDxxxM_BS128K_PS2K_SS128
	depends on IPC_FLASH_NAND 
	bool "Total:128M/256M/512M,Block:128K,Page:2K,Spare:128."
comment "GD5F1GQ4xBxIG,GD5F2GQ4UB9IGR,select above."
	depends on IPC_FLASH_NAND 

config IPC_FLASH_NANDxxxM_BS256K_PS4K_SS256
	depends on IPC_FLASH_NAND 
	bool "Total:512M,Block:256K,Page:4K,Spare:256."
comment "GD5F4GQ4xBxIG,select above."
	depends on IPC_FLASH_NAND 
	
endchoice

config IPC_FLASH_NOR8M
	bool
	default n
	
config IPC_FLASH_NOR16M
	bool
	default n
	
config IPC_FLASH_NAND
	bool
	default n
	
config IPC_FLASHMEDIA
	string
	default "NOR" if IPC_FLASH_NOR8M || IPC_FLASH_NOR16M
	default "NAND" if IPC_FLASH_NAND
	
config IPC_FLASHTSM
	string
	default "8" if IPC_FLASH_NOR8M
	default "16" if IPC_FLASH_NOR16M
	default "128" if IPC_FLASH_NANDxxxM_BS128K_PS2K_SS64 || IPC_FLASH_NANDxxxM_BS128K_PS2K_SS128 || IPC_FLASH_NANDxxxM_BS256K_PS4K_SS256 
#	default "256M" if IPC_FLASH_NANDxxxM_BS128K_PS2K_SS64
#	default "512M" if IPC_FLASH_NANDxxxM_BS128K_PS2K_SS64
	
config IPC_FLASHBSK
	string
	default "128" if IPC_FLASH_NANDxxxM_BS128K_PS2K_SS64 || IPC_FLASH_NANDxxxM_BS128K_PS2K_SS128 
	default "256" if IPC_FLASH_NANDxxxM_BS256K_PS4K_SS256

config IPC_FLASHPSK
	string
	default "2" if IPC_FLASH_NANDxxxM_BS128K_PS2K_SS64 || IPC_FLASH_NANDxxxM_BS128K_PS2K_SS128 
	default "4" if IPC_FLASH_NANDxxxM_BS256K_PS4K_SS256

config IPC_FLASHSSB
	string
	default "64" if IPC_FLASH_NANDxxxM_BS128K_PS2K_SS64
	default "128" if IPC_FLASH_NANDxxxM_BS128K_PS2K_SS128 
	default "256" if IPC_FLASH_NANDxxxM_BS256K_PS4K_SS256

