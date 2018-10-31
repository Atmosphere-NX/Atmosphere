ifeq ($(strip $(DEVKITPRO)),)

PREFIX			:=	aarch64-none-elf-

export CC		:=	$(PREFIX)gcc
export CXX		:=	$(PREFIX)g++
export AS		:=	$(PREFIX)as
export AR		:=	$(PREFIX)gcc-ar
export OBJCOPY	:=	$(PREFIX)objcopy

ISVC=$(or $(VCBUILDHELPER_COMMAND),$(MSBUILDEXTENSIONSPATH32),$(MSBUILDEXTENSIONSPATH))

ifneq (,$(ISVC))
	ERROR_FILTER	:=	2>&1 | sed -e 's/\(.[a-zA-Z]\+\):\([0-9]\+\):/\1(\2):/g'
endif

else
include $(DEVKITPRO)/devkitA64/base_tools
endif

ARCH_SETTING			:= -march=armv8-a -mgeneral-regs-only
ARCH_DEFINES			:= -DMESOSPHERE_ARCH_ARM64
ARCH_CFLAGS				:=
ARCH_CXXFLAGS			:=
ARCH_SOURCE_DIRS 		:= source/arch/arm64
