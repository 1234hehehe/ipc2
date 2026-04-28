
############################################################
# choose autolens-driver or non-autolens
choice 
	prompt "AutoLens Driver"
	default IPC_AUTOLENS_DRIVER_NONE
	
config IPC_AUTOLENS_DRIVER_NONE
	bool "NONE"
	
config IPC_AUTOLENS_DRIVER_DRV8835
	bool "DRV8835"

config IPC_AUTOLENS_DRIVER_MS41908
	bool "MS41908"

config IPC_AUTOLENS_DRIVER_R30440
	bool "R30440"

config IPC_AUTOLENS_DRIVER_TMI8150
	bool "TMI8150"

config IPC_AUTOLENS_DRIVER_XAControlBoard
	bool "XACtrlBoard"

config IPC_AUTOLENS_DRIVER_HSControlBoard
	bool "HSCtrlBoard"

endchoice
# <choose autolens/> ------------------------------------#

############################################################
# choose autolens-model
choice 
	prompt "AutoLens Model"
	
config IPC_AUTOLENS_TYPE_NONE
	bool "NONE"
	depends on IPC_AUTOLENS_DRIVER_NONE
	
config IPC_AUTOLENS_TYPE_AUTO
	bool "AUTO"
	depends on !IPC_AUTOLENS_DRIVER_NONE
	
config IPC_AUTOLENS_TYPE_YT_2812_2MP
	bool "YT_2812_2MP"
	depends on IPC_AUTOLENS_DRIVER_MS41908
	
config IPC_AUTOLENS_TYPE_YT_2812_4MP
	bool "YT_2812_4MP"
	depends on IPC_AUTOLENS_DRIVER_MS41908

config IPC_AUTOLENS_TYPE_RICOM_2812_3MP
	bool "RICOM_2812_3MP"
	depends on IPC_AUTOLENS_DRIVER_MS41908

config IPC_AUTOLENS_TYPE_RICOM_2812_5MP
	bool "RICOM_2812_5MP"
	depends on IPC_AUTOLENS_DRIVER_MS41908

config IPC_AUTOLENS_TYPE_YT_3611_8MP
	bool "YT_3611_4K"
	depends on IPC_AUTOLENS_DRIVER_MS41908

config IPC_AUTOLENS_TYPE_YT_27135_4MP
	bool "YT_27135_4MP"
	depends on IPC_AUTOLENS_DRIVER_MS41908
	
config IPC_AUTOLENS_TYPE_YT_27135_5MP
	bool "YT_27135_5MP"
	depends on IPC_AUTOLENS_DRIVER_MS41908
	
config IPC_AUTOLENS_TYPE_YT_27135_8MP
	bool "YT_27135_8MP"
	depends on IPC_AUTOLENS_DRIVER_MS41908
	
config IPC_AUTOLENS_TYPE_YT_6022_5MP
	bool "YT_6022_5MP"
	depends on IPC_AUTOLENS_DRIVER_MS41908
	
config IPC_AUTOLENS_TYPE_YT_6022_8MP
	bool "YT_6022_8MP"
	depends on IPC_AUTOLENS_DRIVER_MS41908

config IPC_AUTOLENS_TYPE_YS15_3_IR_C3
	bool "YS15_3_IR_C3(2MP)"
	depends on IPC_AUTOLENS_DRIVER_MS41908

config IPC_AUTOLENS_TYPE_YS39_IR
	bool "YS39_IR(5MP)"
	depends on IPC_AUTOLENS_DRIVER_MS41908

config IPC_AUTOLENS_TYPE_YS05_IR_F3_18
	bool "YS05_IR_F3_18(2MP 10bei)"
	depends on IPC_AUTOLENS_DRIVER_MS41908

config IPC_AUTOLENS_TYPE_YS18_IR_D3
	bool "YS18_IR_D3(2MP 18bei)"
	depends on IPC_AUTOLENS_DRIVER_MS41908

config IPC_AUTOLENS_TYPE_YS50_IR_C3
	bool "YS50_IR_C3(5MP 3bei)"
	depends on IPC_AUTOLENS_DRIVER_MS41908

config IPC_AUTOLENS_TYPE_FOCTEK_2812_3M
	bool "FOCTEK_2812_3M(3MP)"
	depends on IPC_AUTOLENS_DRIVER_MS41908

config IPC_AUTOLENS_TYPE_XA_4X
	bool "XA_4X(4MP)"
	depends on IPC_AUTOLENS_DRIVER_XAControlBoard

config IPC_AUTOLENS_TYPE_XA_20X
	bool "XA_20X(5MP)"
	depends on IPC_AUTOLENS_DRIVER_XAControlBoard

config IPC_AUTOLENS_TYPE_HS_4X
	bool "HS_4X(4MP)"
	depends on IPC_AUTOLENS_DRIVER_HSControlBoard

config IPC_AUTOLENS_TYPE_HS_20X
	bool "HS_20X(5MP)"
	depends on IPC_AUTOLENS_DRIVER_HSControlBoard

endchoice
# <choose autolens/> ------------------------------------#

config IPC_AUTOLENS_DRIVER
	string
	default "NONE" if IPC_AUTOLENS_DRIVER_NONE
	default "DRV8835" if IPC_AUTOLENS_DRIVER_DRV8835
	default "MS41908" if IPC_AUTOLENS_DRIVER_MS41908
	default "R30440" if IPC_AUTOLENS_DRIVER_R30440
	default "TMI8150" if IPC_AUTOLENS_DRIVER_TMI8150
	default "XACtrlBoard" if IPC_AUTOLENS_DRIVER_XAControlBoard
	default "HSCtrlBoard" if IPC_AUTOLENS_DRIVER_HSControlBoard

config IPC_AUTOLENS_SUPPORT
	string
	default "0" if IPC_AUTOLENS_DRIVER_NONE
	default "1" if !IPC_AUTOLENS_DRIVER_NONE

config IPC_AUTOLENS_MODEL
	string
	default "NONE" if IPC_AUTOLENS_TYPE_NONE
	default "AUTO" if IPC_AUTOLENS_TYPE_AUTO
	default "YS15_3_IR_C3" if IPC_AUTOLENS_TYPE_YS15_3_IR_C3
	default "RICOM_2812_3MP" if IPC_AUTOLENS_TYPE_RICOM_2812_3MP
	default "RICOM_2812_5MP" if IPC_AUTOLENS_TYPE_RICOM_2812_5MP
	default "YS39_IR" if IPC_AUTOLENS_TYPE_YS39_IR
	default "YT_3611_4K" if IPC_AUTOLENS_TYPE_YT_3611_8MP
	default "YT_27135_4MP" if IPC_AUTOLENS_TYPE_YT_27135_4MP
	default "YT_27135_5MP" if IPC_AUTOLENS_TYPE_YT_27135_5MP
	default "YT_27135_8MP" if IPC_AUTOLENS_TYPE_YT_27135_8MP
	default "YT_6022_5MP" if IPC_AUTOLENS_TYPE_YT_6022_5MP
	default "YT_6022_8MP" if IPC_AUTOLENS_TYPE_YT_6022_8MP
	default "YS05_IR_F3_18" if IPC_AUTOLENS_TYPE_YS05_IR_F3_18
	default "YS18_IR_D3" if IPC_AUTOLENS_TYPE_YS18_IR_D3
	default "YT_2812_2MP" if IPC_AUTOLENS_TYPE_YT_2812_2MP
	default "YT_2812_4MP" if IPC_AUTOLENS_TYPE_YT_2812_4MP
	default "YS50_IR_C3" if IPC_AUTOLENS_TYPE_YS50_IR_C3
	default "FOCTEK_2812_3M" if IPC_AUTOLENS_TYPE_FOCTEK_2812_3M
	default "XA_4X" if IPC_AUTOLENS_TYPE_XA_4X
	default "XA_20X" if IPC_AUTOLENS_TYPE_XA_20X
	default "HS_4X" if IPC_AUTOLENS_TYPE_HS_4X
	default "HS_20X" if IPC_AUTOLENS_TYPE_HS_20X

# <choose autolens/> ------------------------------------#
############################################################
