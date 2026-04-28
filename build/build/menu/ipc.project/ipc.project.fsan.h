
choice 
	prompt "FSAN Project"
	depends on IPC_CUSTOMER_FSAN

config IPC_PROJECT_DEFAULT
	bool "Default Project"
	depends on false

config IPC_PROJECT_FSAN_JZT31N_PERSON
	bool "31N Tiny Smart (16M person tf lens version)"
	depends on IPC_HARDWARE_JZT31N_D11_W386

config IPC_PROJECT_FSAN_JZT31X_PERSON
	bool "31X Tiny Smart (16M person tf lens version)"
	depends on IPC_HARDWARE_JZT31X_D11_W386
endchoice


config IPC_PROJECT
	string
	default "T31N_PERSON" if IPC_PROJECT_FSAN_JZT31N_PERSON
	default "T31X_PERSON" if IPC_PROJECT_FSAN_JZT31X_PERSON
