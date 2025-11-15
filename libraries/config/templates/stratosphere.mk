#---------------------------------------------------------------------------------
# pull in common atmosphere configuration
#---------------------------------------------------------------------------------
include  $(dir $(abspath $(lastword $(MAKEFILE_LIST))))/../common.mk

ifeq ($(ATMOSPHERE_BOARD),nx-hac-001)

#---------------------------------------------------------------------------------
# pull in switch rules
#---------------------------------------------------------------------------------
ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>/devkitpro")
endif

include $(DEVKITPRO)/libnx/switch_rules

endif

#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------
ifeq ($(ATMOSPHERE_BUILD_FOR_DEBUGGING),1)
ATMOSPHERE_OPTIMIZATION_FLAG := -Os
else
ATMOSPHERE_OPTIMIZATION_FLAG := -O2
endif

export DEFINES     = $(ATMOSPHERE_DEFINES) -DATMOSPHERE_IS_STRATOSPHERE -D_GNU_SOURCE
export SETTINGS    = $(ATMOSPHERE_SETTINGS) $(ATMOSPHERE_OPTIMIZATION_FLAG) -Wextra -Werror -Wno-missing-field-initializers -Wno-error=unused-result
export CFLAGS      = $(ATMOSPHERE_CFLAGS) $(SETTINGS) $(DEFINES) $(INCLUDE)
export CXXFLAGS    = $(CFLAGS) $(ATMOSPHERE_CXXFLAGS)
export ASFLAGS     = $(ATMOSPHERE_ASFLAGS) $(SETTINGS) $(DEFINES)

ifeq ($(ATMOSPHERE_BOARD),nx-hac-001)
export CXXREQUIRED := -Wl,--require-defined,__libnx_initheap \
                      -Wl,--require-defined,__libnx_exception_handler \
                      -Wl,--require-defined,__libnx_alloc \
                      -Wl,--require-defined,__libnx_aligned_alloc \
                      -Wl,--require-defined,__libnx_free \
                      -Wl,--require-defined,__appInit \
                      -Wl,--require-defined,__appExit \
                      -Wl,--require-defined,argvSetup

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
			-Wl,--wrap,_ZNSt11logic_errorC2EPKc \
			-Wl,--wrap,exit
else ifeq ($(ATMOSPHERE_BOARD),generic_windows)
export CXXREQUIRED :=
export CXXWRAPS    := -Wl,--wrap,__p__acmdln -Wl,--wrap,_set_invalid_parameter_handler
else
export CXXREQUIRED :=
export CXXWRAPS    :=
endif


ifeq ($(ATMOSPHERE_BOARD),nx-hac-001)
export LDFLAGS     = -specs=$(ATMOSPHERE_LIBRARIES_DIR)/libstratosphere/stratosphere.specs -specs=$(DEVKITPRO)/libnx/switch.specs $(CXXFLAGS) $(CXXWRAPS) $(CXXREQUIRED) -Wl,-Map,$(notdir $*.map) -Wl,-z,relro,-z,now
else ifeq ($(ATMOSPHERE_OS_NAME),macos)
export LDFLAGS     = $(CXXFLAGS) $(CXXWRAPS) $(CXXREQUIRED) -Wl,-map,$(notdir $@.map)
else
export LDFLAGS     = $(CXXFLAGS) $(CXXWRAPS) $(CXXREQUIRED) -Wl,-Map,$(notdir $@.map)
endif

ifeq ($(ATMOSPHERE_COMPILER_NAME),clang)
export LDFLAGS += -fuse-ld=lld
endif

ifeq ($(ATMOSPHERE_BOARD),nx-hac-001)
export LIBS	:= -lstratosphere -lnx
else ifeq ($(ATMOSPHERE_BOARD),generic_windows)
export LIBS	:= -lstratosphere -lwinmm -lws2_32 -lbcrypt -static -static-libgcc -static-libstdc++
else ifeq ($(ATMOSPHERE_BOARD),generic_linux)
export LIBS := -lstratosphere -pthread -lbfd
else ifeq ($(ATMOSPHERE_BOARD),generic_macos)
export LIBS := -lstratosphere -pthread
else
export LIBS := -lstratosphere
endif

#---------------------------------------------------------------------------------
# list of directories containing libraries, this must be the top level containing
# include and lib
#---------------------------------------------------------------------------------
ifeq ($(ATMOSPHERE_BOARD),nx-hac-001)
export LIBDIRS	= $(PORTLIBS) $(LIBNX)
else
export LIBDIRS	=
endif

export AMS_LIBDIRS = $(ATMOSPHERE_LIBRARIES_DIR)/libvapours $(ATMOSPHERE_LIBRARIES_DIR)/libstratosphere

#---------------------------------------------------------------------------------
# stratosphere sysmodules may (but usually do not) have an exefs source dir
#---------------------------------------------------------------------------------
export EXEFS_SRC = exefs_src