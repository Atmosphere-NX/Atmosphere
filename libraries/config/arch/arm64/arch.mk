ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>/devkitpro")
endif

include $(DEVKITPRO)/devkitA64/base_rules

export ATMOSPHERE_DEFINES  += -DATMOSPHERE_ARCH_ARM64
export ATMOSPHERE_SETTINGS += -mtp=soft
export ATMOSPHERE_CFLAGS   +=
export ATMOSPHERE_CXXFLAGS +=
export ATMOSPHERE_ASFLAGS  +=
