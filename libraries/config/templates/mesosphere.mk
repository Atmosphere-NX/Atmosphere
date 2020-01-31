#---------------------------------------------------------------------------------
# pull in common atmosphere configuration
#---------------------------------------------------------------------------------
include  $(dir $(abspath $(lastword $(MAKEFILE_LIST))))/../common.mk

#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------
export DEFINES     := $(ATMOSPHERE_DEFINES) -DATMOSPHERE_IS_MESOSPHERE
export SETTINGS    := $(ATMOSPHERE_SETTINGS) -O2 -mgeneral-regs-only -ffixed-x18 -Werror -fno-non-call-exceptions
export CFLAGS      := $(ATMOSPHERE_CFLAGS) $(SETTINGS) $(DEFINES) $(INCLUDE)
export CXXFLAGS    := $(CFLAGS) $(ATMOSPHERE_CXXFLAGS) -fno-use-cxa-atexit
export ASFLAGS     := $(ATMOSPHERE_ASFLAGS) $(SETTINGS) $(DEFINES)

export LDFLAGS	=	-specs=$(TOPDIR)/$(notdir $(TOPDIR)).specs -nostdlib -nostartfiles -g $(SETTINGS) -Wl,-Map,$(notdir $*.map) -Wl,-z,relro,-z,now

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

export LIBS := -lmesosphere

#---------------------------------------------------------------------------------
# list of directories containing libraries, this must be the top level containing
# include and lib
#---------------------------------------------------------------------------------
LIBDIRS	:= $(ATMOSPHERE_LIBRARIES_DIR)/libvapours $(ATMOSPHERE_LIBRARIES_DIR)/libmesosphere
