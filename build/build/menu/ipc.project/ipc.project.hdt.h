
choice 
	prompt "HDT Project"
	depends on IPC_CUSTOMER_HDT

config IPC_PROJECT_DEFAULT
	bool "Default Project"
	depends on false

config IPC_PROJECT_HDT_H16EV2_MINI
	bool "Stare Function (Mini version)"
	depends on IPC_HARDWARE_H16EV2_D96_Q38_MINI

config IPC_PROJECT_HDT_H16EV2_MINIA
	bool "Stare Function (Mini onvif version)"
	depends on IPC_HARDWARE_H16EV2_D96_Q38_MINIA

config IPC_PROJECT_HDT_H16EV2_PERSON_8M
	bool "Tiny Smart (8M person version)"
	depends on IPC_HARDWARE_H16EV2_D96_Q38_PERSONA

config IPC_PROJECT_HDT_H16EV2_MINI_P2P
	bool "P2P (8M version)"
	depends on IPC_HARDWARE_H16EV2_D96_Q38_PERSONA

config IPC_PROJECT_HDT_H16EV2_PERSON_8M
	bool "Tiny Smart (8M person version)"
	depends on IPC_HARDWARE_H16EV2_D96_Q38_PERSONAN

config IPC_PROJECT_HDT_H16EV2_MINI_P2P
	bool "P2P (8M version)"
	depends on IPC_HARDWARE_H16EV2_D96_Q38_P2P

config IPC_PROJECT_HDT_H16EV2_PERSON
	bool "Tiny Smart (16M person wifi tf lens version)"
	depends on IPC_HARDWARE_H16EV2_D92_Q38_PERSONC

config IPC_PROJECT_HDT_H16EV2_P2P
	bool "Tiny Smart (16M p2p wifi tf lens version)"
	depends on IPC_HARDWARE_H16EV2_D92_Q38_P2P	

config IPC_PROJECT_HDT_H16EV2_FACEC
	bool "Tiny Smart (16M face wifi/tf lens version)"
	depends on IPC_HARDWARE_H16EV2_D92_Q38_FACEC

config IPC_PROJECT_HDT_H16EV2_PERSON2
	bool "Tiny Smart (16M person version)"
	depends on IPC_HARDWARE_H16EV2_D96_Q38_STD

config IPC_PROJECT_HDT_H16EV3_FOLLOW2
	bool "Tiny Smart (16M face tf lens version)"
	depends on IPC_HARDWARE_H16EV3_D95_Q38_FACE

config IPC_PROJECT_HDT_H16EV3_PERSON
	bool "Tiny Smart (16M person tf lens  version)"
	depends on IPC_HARDWARE_H16EV3_D95_Q38_PERSON

config IPC_PROJECT_HDT_H16EV3_P2P
	bool "P2P (16M person tf lens wifi  version)"
	depends on IPC_HARDWARE_H16EV3_D95_Q38_P2P

config IPC_PROJECT_HDT_H16CV5_PERSON
	bool "Person"
	depends on IPC_HARDWARE_H16CV5_D01_Q38_PERSON

config IPC_PROJECT_HDT_H16CV5_FACEC
	bool "Face cap IPC"
	depends on IPC_HARDWARE_H16CV5_D01_Q38_FACEC

config IPC_PROJECT_HDT_H16CV5_FACER
	bool "Face rec IPC"
	depends on IPC_HARDWARE_H16CV5_D01_Q38_FACER

config IPC_PROJECT_HDT_H16CV5_PERSONA
	bool "Person (38 single 2053)"
	depends on IPC_HARDWARE_H16CV5_D01_Q38S_PERSON

config IPC_PROJECT_HDT_H16CV5_MOTOR
	bool "Motor (38 single 2053)"
	depends on IPC_HARDWARE_H16CV5_D01_Q38S_MOTOR

config IPC_PROJECT_HDT_H16CV5_FACECA
	bool "Face cap IPC (38 single 2053)"
	depends on IPC_HARDWARE_H16CV5_D01_Q38S_FACEC

config IPC_PROJECT_HDT_H16CV5_FACERA
	bool "Face rec IPC (38 single 2053)"
	depends on IPC_HARDWARE_H16CV5_D01_Q38S_FACER

config IPC_PROJECT_HDT_H16AV3_P2P
	bool "P2P (no smart)"
	depends on IPC_HARDWARE_H16AV3_D01_Q38

	config IPC_PROJECT_HDT_H16DV3_FACEC
	bool "Face cap IPC(5MP)"
	depends on IPC_HARDWARE_H16DV3_D01_Q38_FACEC

config IPC_PROJECT_HDT_H16DV3_FACER
	bool "Face rec IPC(5MP)"
	depends on IPC_HARDWARE_H16DV3_D01_Q38_FACER

config IPC_PROJECT_HDT_JZT31X_PERSON
	bool "Tiny Smart (16M person tf lens version)"
	depends on IPC_HARDWARE_JZT31X_D95_Q38_PERSON

config IPC_PROJECT_HDT_JZT31X_PERSON
	bool "Tiny Smart (16M person tf lens version)"
	depends on IPC_HARDWARE_JZT31X_D11_Q38_PERSON

config IPC_PROJECT_HDT_JZT31X_PERSON
	bool "Tiny Smart (16M person mini version)"
	depends on IPC_HARDWARE_JZT31X_D12_Q38_PERSON

config IPC_PROJECT_HDT_JZT31X_PERSON
	bool "Tiny Smart (16M person mini version)"
	depends on IPC_HARDWARE_JZT31X_D41_Q38_PERSON

config IPC_PROJECT_HDT_JZT31X_H5
	bool "Tiny Smart with html5 (16M person tf lens version)"
	depends on IPC_HARDWARE_JZT31X_D95_Q38_H5

config IPC_PROJECT_HDT_JZT31X_P2P
	bool "p2p (16M tf lens wifi p2p version)"
	depends on IPC_HARDWARE_JZT31X_D95_Q38_P2P

config IPC_PROJECT_HDT_JZT31X_P2P
	bool "p2p (16M tf lens wifi p2p version)"
	depends on IPC_HARDWARE_JZT31X_D11_Q38_P2P

config IPC_PROJECT_HDT_JZT31X_P2P
	bool "p2p (16M p2p mini version)"
	depends on IPC_HARDWARE_JZT31X_D12_Q38_P2P

config IPC_PROJECT_HDT_JZT31N_PERSON
	bool "Tiny Smart (16M 3MP non-person tf lens version)"
	depends on IPC_HARDWARE_JZT31N_D20_Q38_PERSON

config IPC_PROJECT_HDT_JZT31N_PERSON
	bool "Tiny Smart (16M person tf lens version)"
	depends on IPC_HARDWARE_JZT31N_D96_Q38_PERSON

config IPC_PROJECT_HDT_JZT31N_P2P
	bool "P2P (16M 3MP non-person tf lens version)"
	depends on IPC_HARDWARE_JZT31N_D20_Q38_P2P

config IPC_PROJECT_HDT_JZT31N_P2P
	bool "P2P (16M person tf lens version)"
	depends on IPC_HARDWARE_JZT31N_D96_Q38_P2P

config IPC_PROJECT_HDT_JZT31N_MINI
	bool "Tiny Smart (8M person version)"
	depends on IPC_HARDWARE_JZT31N_D96_Q38_MINI

config IPC_PROJECT_HDT_JZT31N_MINI
	bool "Tiny Smart (8M person version)"
	depends on IPC_HARDWARE_JZT31N_D41_Q38

config IPC_PROJECT_HDT_JZT31N_MINI
	bool "Tiny Smart (8M person version)"
	depends on IPC_HARDWARE_JZT31N_D11_Q38_MINI4MP

config IPC_PROJECT_HDT_JZT31N_MINI
	bool "Tiny Smart (8M person version)"
	depends on IPC_HARDWARE_JZT31N_D43_Q38

config IPC_PROJECT_HDT_JZT31N_MINI
	bool "Tiny Smart (8M person version)"
	depends on IPC_HARDWARE_JZT31N_D44_Q38

config IPC_PROJECT_HDT_JZT31N_MINI
	bool "Tiny Smart (8M person version)"
	depends on IPC_HARDWARE_JZT31N_D45_Q38

config IPC_PROJECT_HDT_JZT31N_MINI_P2P
	bool "Tiny Smart (8M person version)"
	depends on IPC_HARDWARE_JZT31N_D44_Q38_P2P

config IPC_PROJECT_HDT_JZT31N_MINI_P2P
	bool "Tiny Smart (8M person version)"
	depends on IPC_HARDWARE_JZT31N_D11_Q38_MINI4MP_P2P
	
config IPC_PROJECT_HDT_JZT31L_MINI
	bool "Tiny Smart (8M person version)"
	depends on IPC_HARDWARE_JZT31L_D20_Q38_MINI
	
config IPC_PROJECT_HDT_JZT31N_PERSON_MINI
	bool "Tiny Smart (8M 3mp person version)"
	depends on IPC_HARDWARE_JZT31N_D21_Q38_MINI

config IPC_PROJECT_HDT_JZT31N_PERSON_MINI
	bool "Tiny Smart (8M person version)"
	depends on IPC_HARDWARE_JZT31N_D42_Q38

config IPC_PROJECT_HDT_JZT31N_PERSON_MINI
	bool "Tiny Smart (8M 3mp person version)"
	depends on IPC_HARDWARE_JZT31N_D31_Q38

config IPC_PROJECT_HDT_JZT31N_MINI_P2P
	bool "P2P (8M p2p version)"
	depends on IPC_HARDWARE_JZT31N_D01_Q38_P2P

config IPC_PROJECT_HDT_JZT31N_MINI_P2P
	bool "P2P (8M p2p version)"
	depends on IPC_HARDWARE_JZT31N_D02_Q38_P2P

config IPC_PROJECT_HDT_JZT31N_WIFI_DOME
	bool "wifi dome (16M wifi p2p version)"
	depends on IPC_HARDWARE_JZT31N_D11_X40_WIFIDOME

config IPC_PROJECT_HDT_JZT31N_GB28181_DOME
	bool "gb28181 dome (16M gb28181 dome version)"
	depends on IPC_HARDWARE_JZT31N_D11_X40

config IPC_PROJECT_HDT_JZT31N_P2P_HSDOME
	bool "P2P_HS (16M hs p2p version)"
	depends on IPC_HARDWARE_JZT31N_D02_Q38_P2P_HSDOME

config IPC_PROJECT_HDT_JZT31X_P2P_HSDOME
	bool "P2P_HS (16M hs p2p version)"
	depends on IPC_HARDWARE_JZT31X_D01_Q38_P2P_HSDOME

config IPC_PROJECT_HDT_JZT31X_P2P_HSDOME
	bool "P2P_HS (16M hs p2p version)"
	depends on IPC_HARDWARE_JZT31X_D13_Q38_P2P_HSDOME

config IPC_PROJECT_HDT_JZT31N_P2P_HSWIFIDOME
	bool "P2P_HS (16M hs p2p version)"
	depends on IPC_HARDWARE_JZT31N_D02_Q38_P2P_HSWIFIDOME

config IPC_PROJECT_HDT_JZT31X_P2P_HSWIFIDOME
	bool "P2P_HS (16M hs p2p version)"
	depends on IPC_HARDWARE_JZT31X_D01_Q38_P2P_HSWIFIDOME

config IPC_PROJECT_HDT_JZT40XP_PERSON
	bool "8MP (128M nand 8mp person version)"
	depends on IPC_HARDWARE_JZT40XP_D11_Q38

config IPC_PROJECT_HDT_JZT40XP_PERSONV2
	bool "8MP (16M nor 8mp person vihicle version)"
	depends on IPC_HARDWARE_JZT40XP_D21_Q38

config IPC_PROJECT_HDT_JZT40N_PERSON_8MP
	bool "8MP (16M nor 8mp person version)"
	depends on IPC_HARDWARE_JZT40N_D13_Q38_8MP

config IPC_PROJECT_HDT_JZT40N_PERSON_4KP2P
	bool "8MP P2P(16M nor 8mp person version)"
	depends on IPC_HARDWARE_JZT40N_D13_Q38_4KP2P

config IPC_PROJECT_HDT_JZT40N_PERSON_5MP
	bool "5MP (16M nor 5mp person version)"
	depends on IPC_HARDWARE_JZT40N_D13_Q38_5MP

config IPC_PROJECT_HDT_JZT40N_PERSON
	bool "5MP (16M nor 1080P HD person version)"
	depends on IPC_HARDWARE_JZT40N_D13_Q38

config IPC_PROJECT_HDT_JZT40N_2EYESSTITCH
	bool "2MPx2 (16M nor 1080Px2 stitch version)"
	depends on IPC_HARDWARE_JZT40N_D31_X65_STITCH

config IPC_PROJECT_HDT_JZT41L_PERSON
	bool "3MP (16M nor 3MP HD person version)"
	depends on IPC_HARDWARE_JZT41L_D21_Q38

config IPC_PROJECT_HDT_JZT41L_ALARM
	bool "3/4MP (16M nor 3/4MP HD alarm version)"
	depends on IPC_HARDWARE_JZT41L_D31_Q38_ALARM

config IPC_PROJECT_HDT_JZT41L_ALARM
	bool "5MP (16M nor 5MP HD alarm version)"
	depends on IPC_HARDWARE_JZT41L_D31_Q38

config IPC_PROJECT_HDT_JZT41L_PERSON
	bool "5MP (16M nor 5MP HD person version)"
	depends on IPC_HARDWARE_JZT41L_D31_Q38

config IPC_PROJECT_HDT_JZT41N_ALARM
	bool "4/5MP (16M nor 4/5MP HD alarm version)"
	depends on IPC_HARDWARE_JZT41N_D21_Q38 || IPC_HARDWARE_JZT41N_D31_Q38_ALARM

config IPC_PROJECT_HDT_JZT41N_DOME
	bool "4/5MP (16M nor dome version)"
	depends on IPC_HARDWARE_JZT41N_D32_Q38_DOME

config IPC_PROJECT_HDT_JZT41N_ALARM_FULL
	bool "5/8MP (16M nor full alarm version)"
	depends on IPC_HARDWARE_JZT41N_D33_Q40_FULL

config IPC_PROJECT_HDT_JZT41N_ALARM_FULL
	bool "5/8MP (16M nor full alarm version)"
	depends on IPC_HARDWARE_JZT41N_D33_Q40_FULL_DOME

config IPC_PROJECT_HDT_JZT41N_2XDOME
	bool "5/8MP (16M nor full alarm version)"
	depends on IPC_HARDWARE_JZT41N_D41_X40_2XDOME

config IPC_PROJECT_HDT_JZT41N_ALARM_FULL
	bool "2MP (16M nor aiisp alarm full version)"
	depends on IPC_HARDWARE_JZT41N_D42_Q38_AIISP

config IPC_PROJECT_HDT_JZT41N_ALARM
	bool "4MP (16M nor aiisp alarm version)"
	depends on IPC_HARDWARE_JZT41N_D42_Q38_AIISP4MP

config IPC_PROJECT_HDT_JZT40XP_MOTOR
	bool "8MP (128M nand 8mp version)"
	depends on IPC_HARDWARE_JZT40XP_D12_W386_MOTOR

config IPC_PROJECT_HDT_JZT31N_D01_Q38_WIFIP2P
	bool "16MP (wifi p2p version)"
	depends on IPC_HARDWARE_JZT31N_D01_Q38_WIFIP2P

config IPC_PROJECT_HDT_JZT31X_D11_Q38_WIFIP2P
	bool "16MP (wifi p2p version)"
	depends on IPC_HARDWARE_JZT31X_D11_Q38_WIFIP2P

config IPC_PROJECT_HDT_JZT32L_PERSON
	bool "5MP (8M nor 5MP HD person version)"
	depends on IPC_HARDWARE_JZT32L_D41_Q38

config IPC_PROJECT_HDT_JZT32L_PERSON
	bool "5MP (8M nor 5MP HD person version)"
	depends on IPC_HARDWARE_JZT32L_D51_Q38

config IPC_PROJECT_HDT_JZT32L_PERSON
	bool "5MP (8M nor 5MP HD person version)"
	depends on IPC_HARDWARE_JZT32L_D53_Q38

config IPC_PROJECT_HDT_JZT32N_FULL_ALARM
	bool "6/8MP (8M nor 6/8MP HD person version)"
	depends on IPC_HARDWARE_JZT32N_D41_Q38

config IPC_PROJECT_HDT_JZT32N_FULL_ALARM
	bool "2MP (2MP HD 2eye person version)"
	depends on IPC_HARDWARE_JZT32N_D52_X40_2EYE

config IPC_PROJECT_HDT_JZT32L_PERSON
	bool "5MP (8M nor 5MP HD person version)"
	depends on IPC_HARDWARE_JZT33L_D51_Q38

config IPC_PROJECT_HDT_H16EV3_XZC
	bool "Tiny Smart (16M xzc version)"
	depends on IPC_HARDWARE_H16EV3_D11_W3711_XZC

config IPC_PROJECT_HDT_JZT32N_AOV
	bool "Tiny Smart (4M xzc version)"
	depends on IPC_HARDWARE_JZT32N_D51_Q52

config IPC_PROJECT_HDT_JZT33N_FULL_ALARM
	bool "6MP (alarm version)"
	depends on IPC_HARDWARE_JZT33N_D51_Q38
endchoice


config IPC_PROJECT
	string
	default "FACEC" if IPC_PROJECT_HDT_H16EV2_FACEC
	default "MINI" if IPC_PROJECT_HDT_H16EV2_MINI
	default "MINIA" if IPC_PROJECT_HDT_H16EV2_MINIA
	default "PERSON" if IPC_PROJECT_HDT_H16EV2_PERSON
	default "PERSON_8M" if IPC_PROJECT_HDT_H16EV2_PERSON_8M
	default "MINI_P2P" if IPC_PROJECT_HDT_H16EV2_MINI_P2P
	default "PERSON2" if IPC_PROJECT_HDT_H16EV2_PERSON2
	default "FOLLOW2" if IPC_PROJECT_HDT_H16EV3_FOLLOW2
	default "EV3_PERSON" if IPC_PROJECT_HDT_H16EV3_PERSON
	default "EV3_P2P" if IPC_PROJECT_HDT_H16EV3_P2P
	default "EV2_P2P" if IPC_PROJECT_HDT_H16EV2_P2P
	default "PERSON" if IPC_PROJECT_HDT_H16CV5_PERSON
	default "FACEC" if IPC_PROJECT_HDT_H16CV5_FACEC
	default "FACER" if IPC_PROJECT_HDT_H16CV5_FACER
	default "PERSON" if IPC_PROJECT_HDT_H16CV5_PERSONA
	default "MOTOR" if IPC_PROJECT_HDT_H16CV5_MOTOR
	default "FACEC" if IPC_PROJECT_HDT_H16CV5_FACECA
	default "FACER" if IPC_PROJECT_HDT_H16CV5_FACERA
	default "AV3_P2P" if IPC_PROJECT_HDT_H16AV3_P2P
	default "FACEC" if IPC_PROJECT_HDT_H16DV3_FACEC
	default "FACER" if IPC_PROJECT_HDT_H16DV3_FACER
	default "T40_PERSON" if IPC_PROJECT_HDT_JZT40XP_PERSON
	default "T40XP_PERSON" if IPC_PROJECT_HDT_JZT40XP_PERSONV2
	default "T40N_PERSON_4K" if IPC_PROJECT_HDT_JZT40N_PERSON_8MP
	default "T40N_PERSON_5MP" if IPC_PROJECT_HDT_JZT40N_PERSON_5MP
	default "T40N_PERSON" if IPC_PROJECT_HDT_JZT40N_PERSON
	default "T40_MOTOR" if IPC_PROJECT_HDT_JZT40XP_MOTOR
	default "T40N_PERSON_4KP2P" if IPC_PROJECT_HDT_JZT40N_PERSON_4KP2P
	default "T40N_2EYESSTITCH" if IPC_PROJECT_HDT_JZT40N_2EYESSTITCH

	default "T31X_PERSON" if IPC_PROJECT_HDT_JZT31X_PERSON
	default "HTML5" if IPC_PROJECT_HDT_JZT31X_H5
	default "T31X_P2P" if IPC_PROJECT_HDT_JZT31X_P2P
	default "T31N_PERSON" if IPC_PROJECT_HDT_JZT31N_PERSON
	default "T31N_P2P" if IPC_PROJECT_HDT_JZT31N_P2P
	default "T31N_PERSON_8M" if IPC_PROJECT_HDT_JZT31N_MINI
	default "T31L_PERSON_8M" if IPC_PROJECT_HDT_JZT31L_MINI
	default "T31N_PERSON_MINI" if IPC_PROJECT_HDT_JZT31N_PERSON_MINI
	default "T31N_MINI_P2P" if IPC_PROJECT_HDT_JZT31N_MINI_P2P
	default "T31N_WIFI_DOME" if IPC_PROJECT_HDT_JZT31N_WIFI_DOME
	default "T31N_P2P_HS" if IPC_PROJECT_HDT_JZT31N_P2P_HSDOME
	default "T31X_P2P_HS" if IPC_PROJECT_HDT_JZT31X_P2P_HSDOME
	default "T31N_P2P_HSWIFI" if IPC_PROJECT_HDT_JZT31N_P2P_HSWIFIDOME
	default "T31X_P2P_HSWIFI" if IPC_PROJECT_HDT_JZT31X_P2P_HSWIFIDOME
	default "T31N_GB28181_DOME" if IPC_PROJECT_HDT_JZT31N_GB28181_DOME
	default "T31N_P2P_WIFI" if IPC_PROJECT_HDT_JZT31N_D01_Q38_WIFIP2P
	default "T31X_P2P_WIFI" if IPC_PROJECT_HDT_JZT31X_D11_Q38_WIFIP2P

	default "T41L_PERSON" if IPC_PROJECT_HDT_JZT41L_PERSON
	default "T41L_ALARM"  if IPC_PROJECT_HDT_JZT41L_ALARM
	default "T41N_ALARM"  if IPC_PROJECT_HDT_JZT41N_ALARM
	default "T41N_DOME"		if IPC_PROJECT_HDT_JZT41N_DOME
	default "T41N_FULL_ALARM"		if IPC_PROJECT_HDT_JZT41N_ALARM_FULL
	default "T41N_FULL_ALARM"		if IPC_PROJECT_HDT_JZT41N_ALARM_FULL_DOME
	default "T41N_2XDOME"		if IPC_PROJECT_HDT_JZT41N_2XDOME
	default "T41N_FULL_ALARM"		if IPC_PROJECT_HDT_JZT41N_AIISP
	default "T41N_ALARM"		if IPC_PROJECT_HDT_JZT41N_AIISP4MP

	default "T32L_PERSON"		if IPC_PROJECT_HDT_JZT32L_PERSON
	default "T32N_FULL_ALARM"		if IPC_PROJECT_HDT_JZT32N_FULL_ALARM

	default "EV3_XZC" if IPC_PROJECT_HDT_H16EV3_XZC
	default "T32N_AOV"		if IPC_PROJECT_HDT_JZT32N_AOV

	default "T33N_FULL_ALARM"		if IPC_PROJECT_HDT_JZT33N_FULL_ALARM

