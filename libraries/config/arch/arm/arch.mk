ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>/devkitpro")
endif

include $(DEVKITPRO)/devkitARM/base_rules

export ATMOSPHERE_DEFINES  += -DATMOSPHERE_ARCH_ARM
export ATMOSPHERE_SETTINGS +=
export ATMOSPHERE_CFLAGS   +=
export ATMOSPHERE_CXXFLAGS +=
export ATMOSPHERE_ASFLAGS  +=
