ifeq ($(strip $(ATMOSPHERE_OS_NAME)),horizon)

ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>/devkitpro")
endif

include $(DEVKITPRO)/devkitA64/base_rules

else

include $(ATMOSPHERE_ARCH_MAKE_DIR)/base_rules

endif

export ATMOSPHERE_DEFINES  += -DATMOSPHERE_ARCH_ARM64
export ATMOSPHERE_SETTINGS +=
export ATMOSPHERE_CFLAGS   +=
export ATMOSPHERE_CXXFLAGS +=
export ATMOSPHERE_ASFLAGS  +=

ifeq ($(strip $(ATMOSPHERE_OS_NAME)),horizon)
export ATMOSPHERE_SETTINGS += -mtp=soft
endif