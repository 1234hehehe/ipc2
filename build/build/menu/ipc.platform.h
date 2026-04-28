
## Usually, keep options in the sort of alphabets.

############################################################
# <CPU Platform> ------------------------------------#
choice 
	prompt "CPU Platform"
	default IPC_CPUFAMILY_JZT33

config IPC_PLATFORM_JZT33N
	bool "JZT33N (JZT33 family)"
	select IPC_CPUFAMILY_JZT33

endchoice


config IPC_PLATFORM
	string
	default "HI3516CV500" if IPC_PLATFORM_HI3516CV500
	default "HI3516DV300" if IPC_PLATFORM_HI3516DV300
	default "HI3516AV300" if IPC_PLATFORM_HI3516AV300
	default "HI3516EV200" if IPC_PLATFORM_HI3516EV200
	default "HI3516EV300" if IPC_PLATFORM_HI3516EV300
	default "HI3518EV300" if IPC_PLATFORM_HI3518EV300
	default "JZT30" 			if IPC_PLATFORM_JZT30
	default "JZT31L"			if IPC_PLATFORM_JZT31L
	default "JZT31N"			if IPC_PLATFORM_JZT31N
	default "JZT31X"			if IPC_PLATFORM_JZT31X
	default "JZT32L"			if IPC_PLATFORM_JZT32L
	default "JZT32N"			if IPC_PLATFORM_JZT32N
	default "JZT33L"			if IPC_PLATFORM_JZT33L
	default "JZT33N"			if IPC_PLATFORM_JZT33N
	default "JZT40XP"			if IPC_PLATFORM_JZT40XP
	default "JZT40N"			if IPC_PLATFORM_JZT40N
	default "JZT41N"			if IPC_PLATFORM_JZT41N
	default "JZT41L"			if IPC_PLATFORM_JZT41L

# <CPU Platform/> ------------------------------------#



############################################################
# <CPU Family> ------------------------------------#
	
config IPC_CPUFAMILY_HI3516CV500
	depends on IPC_PLATFORM_HI3516CV500 || IPC_PLATFORM_HI3516DV300 || IPC_PLATFORM_HI3516AV300
    bool
    default y if IPC_PLATFORM_HI3516CV500 || IPC_PLATFORM_HI3516DV300 || IPC_PLATFORM_HI3516AV300

config IPC_CPUFAMILY_HI3516EV200
	depends on IPC_PLATFORM_HI3516EV200 || IPC_PLATFORM_HI3516EV300 || IPC_PLATFORM_HI3518EV300
    bool
    default y if IPC_PLATFORM_HI3516EV200 || IPC_PLATFORM_HI3516EV300 || IPC_PLATFORM_HI3518EV300

config IPC_CPUFAMILY_JZT30
	depends on IPC_PLATFORM_JZT30 || IPC_PLATFORM_JZT31L || IPC_PLATFORM_JZT31N || IPC_PLATFORM_JZT31X
    bool
    default y if IPC_PLATFORM_JZT30 || IPC_PLATFORM_JZT31L || IPC_PLATFORM_JZT31N || IPC_PLATFORM_JZT31X

config IPC_CPUFAMILY_JZT32
	depends on IPC_PLATFORM_JZT32L || IPC_PLATFORM_JZT32N
    bool
    default y if IPC_PLATFORM_JZT32L || IPC_PLATFORM_JZT32N

config IPC_CPUFAMILY_JZT33
	depends on IPC_PLATFORM_JZT33L || IPC_PLATFORM_JZT33N
    bool
    default y if IPC_PLATFORM_JZT33L || IPC_PLATFORM_JZT33N

config IPC_CPUFAMILY_JZT40
	depends on IPC_PLATFORM_JZT40XP || IPC_PLATFORM_JZT40N
    bool
    default y if IPC_PLATFORM_JZT40XP || IPC_PLATFORM_JZT40N

config IPC_CPUFAMILY_JZT41
	depends on IPC_PLATFORM_JZT41N || IPC_PLATFORM_JZT41L
    bool
    default y if IPC_PLATFORM_JZT41L || IPC_PLATFORM_JZT41N

config IPC_CPUFAMILY
	string
	default "HI3516CV500" if IPC_CPUFAMILY_HI3516CV500
	default "HI3516EV200" if IPC_CPUFAMILY_HI3516EV200
	default "JZT30"				if IPC_CPUFAMILY_JZT30
	default "JZT32"				if IPC_CPUFAMILY_JZT32
	default "JZT33"				if IPC_CPUFAMILY_JZT33
	default "JZT40"				if IPC_CPUFAMILY_JZT40
	default "JZT41"				if IPC_CPUFAMILY_JZT41

# <CPU Family/> ------------------------------------#


############################################################
# <Cross-Compiler> ------------------------------------#

config IPC_COMPILER
	string
	default "arm-himix200-linux-" if IPC_CPUFAMILY_HI3516CV500
	default "arm-himix100-linux-" if IPC_CPUFAMILY_HI3516EV200
	default "mips-linux-gnu-" 		if IPC_CPUFAMILY_JZT30
	default "mips-t32-linux-gnu-" if IPC_CPUFAMILY_JZT32
	default "mips-t33-linux-gnu-" if IPC_CPUFAMILY_JZT33
	default "mips-t40-linux-gnu-" if IPC_CPUFAMILY_JZT40
	default "mips-t41-linux-gnu-" if IPC_CPUFAMILY_JZT41

# <Cross-Compiler/> ------------------------------------#


############################################################
# <SDK version> ------------------------------------#
choice 
	prompt "SDK Version"

config IPC_SDKVERSION_DEFV
	bool "Default Version V010"
	depends on IPC_CPUFAMILY_HI3516CV500

config IPC_SDKVERSION_DEFV
	bool "Default Version V003"
	depends on IPC_CPUFAMILY_HI3516EV200

config IPC_SDKVERSION_V472
	bool "V472"
	depends on IPC_CPUFAMILY_JZT30

config IPC_SDKVERSION_V540
	bool "V540"
	depends on IPC_CPUFAMILY_JZT30

config IPC_SDKVERSION_V100
	bool "V100"
	depends on IPC_CPUFAMILY_JZT32 || IPC_CPUFAMILY_JZT33
	
config IPC_SDKVERSION_V101
	bool "V101"
	depends on IPC_CPUFAMILY_JZT32

config IPC_SDKVERSION_V103
	bool "V103"
	depends on IPC_CPUFAMILY_JZT40
	
config IPC_SDKVERSION_V104
	bool "V104"
	depends on IPC_CPUFAMILY_JZT40
	
config IPC_SDKVERSION_V110
	bool "V110"
	depends on IPC_CPUFAMILY_JZT40 || IPC_CPUFAMILY_JZT41

config IPC_SDKVERSION_V120
	bool "V120"
	depends on IPC_CPUFAMILY_JZT40

config IPC_SDKVERSION_V120GK
	bool "V120GK"
	depends on IPC_CPUFAMILY_JZT41

config IPC_SDKVERSION_V0A0
	bool "V0A0"
	depends on false

config IPC_SDKVERSION_V090
	bool "V090"
	depends on false

config IPC_SDKVERSION_V080
	bool "V080"
	depends on false

config IPC_SDKVERSION_V070
	bool "V070"
	depends on false

config IPC_SDKVERSION_V060
	bool "V060"
	depends on false

config IPC_SDKVERSION_V050
	bool "V050"
	depends on IPC_CPUFAMILY_HI3519

config IPC_SDKVERSION_V040
	bool "V040"
	depends on false

config IPC_SDKVERSION_V030
	bool "V030"
	depends on false
	
config IPC_SDKVERSION_V020
	bool "V020"
	depends on IPC_CPUFAMILY_HI3516CV500
	
config IPC_SDKVERSION_V010
	bool "V010"
	depends on IPC_CPUFAMILY_HI3516EV200 || IPC_CPUFAMILY_HI3516CV500
	
config IPC_SDKVERSION_V011
	bool "V011"
	depends on IPC_CPUFAMILY_HI3516CV500

config IPC_SDKVERSION_V012
	bool "V012"
	depends on IPC_CPUFAMILY_HI3516EV200

endchoice


config IPC_SDKVERSION
	string
	default "DEFVER" if IPC_SDKVERSION_DEFV
	default "V010" if IPC_SDKVERSION_V010
	default "V020" if IPC_SDKVERSION_V020
	default "V030" if IPC_SDKVERSION_V030
	default "V040" if IPC_SDKVERSION_V040
	default "V050" if IPC_SDKVERSION_V050
	default "V060" if IPC_SDKVERSION_V060
	default "V070" if IPC_SDKVERSION_V070
	default "V080" if IPC_SDKVERSION_V080
	default "V090" if IPC_SDKVERSION_V090
	default "V0A0" if IPC_SDKVERSION_V0A0
	default "V100" if IPC_SDKVERSION_V100
	default "V101" if IPC_SDKVERSION_V101
	default "V102" if IPC_SDKVERSION_V102
	default "V003" if IPC_SDKVERSION_V003
	default "V011" if IPC_SDKVERSION_V011
	default "V012" if IPC_SDKVERSION_V012
	default "V103" if IPC_SDKVERSION_V103
	default "V104" if IPC_SDKVERSION_V104
	default "V110" if IPC_SDKVERSION_V110
	default "V120" if IPC_SDKVERSION_V120
	default "V120GK" if IPC_SDKVERSION_V120GK
	default "V472" if IPC_SDKVERSION_V472
	default "V540" if IPC_SDKVERSION_V540

# <SDK version/> ------------------------------------#

