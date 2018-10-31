COMMON_DEFINES		:=	-DBOOST_DISABLE_ASSERTS
COMMON_SOURCES_DIRS	:=	source/core source/interfaces source/interrupts source/kresources\
						source/processes source/threading source
COMMON_SETTING		:=	-fPIE -g -nostdlib
COMMON_CFLAGS		:=	-Wall -Werror -O2 -ffunction-sections -fdata-sections -fno-strict-aliasing -fwrapv\
						-fno-asynchronous-unwind-tables -fno-unwind-tables -fno-stack-protector
COMMON_CXXFLAGS		:=	-fno-rtti -fno-exceptions -std=gnu++17
COMMON_ASFLAGS		:=
COMMON_LDFLAGS		:=	-Wl,-Map,out.map
