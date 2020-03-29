#---------------------------------------------------------------------------------
# pull in common atmosphere configuration
#---------------------------------------------------------------------------------
include  $(dir $(abspath $(lastword $(MAKEFILE_LIST))))/../common.mk

#---------------------------------------------------------------------------------
# pull in switch rules
#---------------------------------------------------------------------------------
ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>/devkitpro")
endif

include $(DEVKITPRO)/libnx/switch_rules

#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------
export DEFINES     = $(ATMOSPHERE_DEFINES) -DATMOSPHERE_IS_STRATOSPHERE -D_GNU_SOURCE
export SETTINGS    = $(ATMOSPHERE_SETTINGS) -O2
export CFLAGS      = $(ATMOSPHERE_CFLAGS) $(SETTINGS) $(DEFINES) $(INCLUDE)
export CXXFLAGS    = $(CFLAGS) $(ATMOSPHERE_CXXFLAGS)
export ASFLAGS     = $(ATMOSPHERE_ASFLAGS) $(SETTINGS) $(DEFINES)

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

export LDFLAGS     = -specs=$(DEVKITPRO)/libnx/switch.specs $(SETTINGS) $(CXXWRAPS) -Wl,-Map,$(notdir $*.map)

export LIBS	= -lstratosphere -lnx

#---------------------------------------------------------------------------------
# list of directories containing libraries, this must be the top level containing
# include and lib
#---------------------------------------------------------------------------------
export LIBDIRS	= $(PORTLIBS) $(LIBNX) $(ATMOSPHERE_LIBRARIES_DIR)/libvapours $(ATMOSPHERE_LIBRARIES_DIR)/libstratosphere

#---------------------------------------------------------------------------------
# stratosphere sysmodules may (but usually do not) have an exefs source dir
#---------------------------------------------------------------------------------
export EXEFS_SRC = exefs_src