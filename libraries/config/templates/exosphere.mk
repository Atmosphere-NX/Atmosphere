#---------------------------------------------------------------------------------
# pull in common atmosphere configuration
#---------------------------------------------------------------------------------
include  $(dir $(abspath $(lastword $(MAKEFILE_LIST))))/../common.mk

#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------
ifeq ($(strip $(ATMOSPHERE_ARCH_NAME)),arm64)
DEFINES     := $(ATMOSPHERE_DEFINES) -DATMOSPHERE_IS_EXOSPHERE
SETTINGS    := $(ATMOSPHERE_SETTINGS) -mgeneral-regs-only -ffixed-x18 -Os -Wextra -Werror -fno-non-call-exceptions
CFLAGS      := $(ATMOSPHERE_CFLAGS) $(SETTINGS) $(DEFINES) $(INCLUDE)
CXXFLAGS    := $(CFLAGS) $(ATMOSPHERE_CXXFLAGS) -fno-use-cxa-atexit
ASFLAGS     := $(ATMOSPHERE_ASFLAGS) $(SETTINGS)
else ifeq ($(strip $(ATMOSPHERE_ARCH_NAME)),arm)
DEFINES     := $(ATMOSPHERE_DEFINES) -DATMOSPHERE_IS_EXOSPHERE
SETTINGS    := $(ATMOSPHERE_SETTINGS) -Os -Werror -fno-non-call-exceptions
CFLAGS      := $(ATMOSPHERE_CFLAGS) $(SETTINGS) $(DEFINES) $(INCLUDE)
CXXFLAGS    := $(CFLAGS) $(ATMOSPHERE_CXXFLAGS) -fno-use-cxa-atexit
ASFLAGS     := $(ATMOSPHERE_ASFLAGS) $(SETTINGS)
endif

export LDFLAGS	=	-specs=$(TOPDIR)/$(notdir $(TOPDIR)).specs -fno-asynchronous-unwind-tables -fno-unwind-tables -fno-exceptions -fno-rtti -fno-use-cxa-atexit -nostdlib -nostartfiles -g $(SETTINGS) -Wl,-Map,$(notdir $*.map) -Wl,-z,relro,-z,now

export CXXWRAPS := -Wl,--wrap,__cxa_pure_virtual \
			-Wl,--wrap,__cxa_throw \
			-Wl,--wrap,__cxa_rethrow \
			-Wl,--wrap,__cxa_allocate_exception \
			-Wl,--wrap,__cxa_free_exception \
			-Wl,--wrap,__cxa_begin_catch \
			-Wl,--wrap,__cxa_end_catch \
			-Wl,--wrap,__cxa_call_unexpected \
			-Wl,--wrap,__cxa_call_terminate \
			-Wl,--wrap,__gxx_personality_v0 \
			-Wl,--wrap,_Unwind_Resume \
			-Wl,--wrap,_ZSt19__throw_logic_errorPKc \
			-Wl,--wrap,_ZSt20__throw_length_errorPKc \
			-Wl,--wrap,_ZNSt11logic_errorC2EPKc

export LIBS := -l$(LIBEXOSPHERE_NAME)

#---------------------------------------------------------------------------------
# list of directories containing libraries, this must be the top level containing
# include and lib
#---------------------------------------------------------------------------------
LIBDIRS	:= $(ATMOSPHERE_LIBRARIES_DIR)/libvapours $(ATMOSPHERE_LIBRARIES_DIR)/libexosphere
