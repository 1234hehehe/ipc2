
choice 
    prompt "Hardware Scheme"

config IPC_HARDWARE_JZT33N_D51_Q38
	depends on IPC_PLATFORM_JZT33N
	bool "JZT33N_D51_Q38"
	select IPC_FLASH_NOR16M

endchoice

config IPC_HARDWARE
	string
	default "JZT33N_D51_Q38" if IPC_HARDWARE_JZT33N_D51_Q38


config IPC_HWID
	string
	default "03a2 03a8" if IPC_HARDWARE_JZT33N_D51_Q38
