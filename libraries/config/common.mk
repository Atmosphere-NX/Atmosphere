#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------
DIR_WILDCARD=$(foreach d,$(wildcard $(1:=/*)),$(if $(wildcard $d/.),$(call DIR_WILDCARD,$d) $d,))

export ATMOSPHERE_CONFIG_MAKE_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
export ATMOSPHERE_LIBRARIES_DIR   := $(ATMOSPHERE_CONFIG_MAKE_DIR)/..

ifeq ($(strip $(ATMOSPHERE_BOARD)),)
export ATMOSPHERE_BOARD := nx-hac-001

ifeq ($(strip $(ATMOSPHERE_CPU)),)
export ATMOSPHERE_CPU            := arm-cortex-a57
endif

endif

ATMOSPHERE_BUILD_SETTINGS ?=

export ATMOSPHERE_DEFINES  := -DATMOSPHERE
export ATMOSPHERE_SETTINGS := -fPIE -g $(ATMOSPHERE_BUILD_SETTINGS)
export ATMOSPHERE_CFLAGS   := -Wall -ffunction-sections -fdata-sections -fno-strict-aliasing -fwrapv  \
                              -fno-asynchronous-unwind-tables -fno-unwind-tables -fno-stack-protector \
                              -Wno-format-truncation -Wno-format-zero-length -Wno-stringop-truncation

export ATMOSPHERE_CXXFLAGS := -fno-rtti -fno-exceptions -std=gnu++20
export ATMOSPHERE_ASFLAGS  :=


ifeq ($(ATMOSPHERE_BOARD),nx-hac-001)

ifeq ($(ATMOSPHERE_CPU),arm-cortex-a57)
export ATMOSPHERE_ARCH_DIR   := arm64
export ATMOSPHERE_BOARD_DIR  := nintendo/nx
export ATMOSPHERE_OS_DIR     := horizon

export ATMOSPHERE_ARCH_NAME  := arm64
export ATMOSPHERE_BOARD_NAME := nintendo_nx
export ATMOSPHERE_OS_NAME    := horizon

export ATMOSPHERE_CPU_EXTENSIONS := arm_crypto_extension aarch64_crypto_extension
else ifeq ($(ATMOSPHERE_CPU),arm7tdmi)
export ATMOSPHERE_ARCH_DIR   := arm
export ATMOSPHERE_BOARD_DIR  := nintendo/nx_bpmp
export ATMOSPHERE_OS_DIR     := horizon

export ATMOSPHERE_ARCH_NAME  := arm
export ATMOSPHERE_BOARD_NAME := nintendo_nx
export ATMOSPHERE_OS_NAME    := horizon

export ATMOSPHERE_CPU_EXTENSIONS :=
endif

endif

ifeq ($(ATMOSPHERE_CPU),arm-cortex-a57)
export ATMOSPHERE_CPU_DIR    := cortex_a57
export ATMOSPHERE_CPU_NAME   := arm_cortex_a57
endif

ifeq ($(ATMOSPHERE_CPU),arm7tdmi)
export ATMOSPHERE_CPU_DIR    := arm7tdmi
export ATMOSPHERE_CPU_NAME   := arm7tdmi
endif


export ATMOSPHERE_ARCH_MAKE_DIR  := $(ATMOSPHERE_CONFIG_MAKE_DIR)/arch/$(ATMOSPHERE_ARCH_DIR)
export ATMOSPHERE_BOARD_MAKE_DIR := $(ATMOSPHERE_CONFIG_MAKE_DIR)/board/$(ATMOSPHERE_BOARD_DIR)
export ATMOSPHERE_OS_MAKE_DIR    := $(ATMOSPHERE_CONFIG_MAKE_DIR)/os/$(ATMOSPHERE_OS_DIR)
export ATMOSPHERE_CPU_MAKE_DIR   := $(ATMOSPHERE_ARCH_MAKE_DIR)/cpu/$(ATMOSPHERE_CPU_DIR)

export ATMOSPHERE_LIBRARY_DIR := lib_$(ATMOSPHERE_BOARD_NAME)_$(ATMOSPHERE_ARCH_NAME)
export ATMOSPHERE_BUILD_DIR := build_$(ATMOSPHERE_BOARD_NAME)_$(ATMOSPHERE_ARCH_NAME)

include $(ATMOSPHERE_ARCH_MAKE_DIR)/arch.mk
include $(ATMOSPHERE_BOARD_MAKE_DIR)/board.mk
include $(ATMOSPHERE_OS_MAKE_DIR)/os.mk
include $(ATMOSPHERE_CPU_MAKE_DIR)/cpu.mk

#---------------------------------------------------------------------------------
# get atmosphere git revision information
#---------------------------------------------------------------------------------
export ATMOSPHERE_GIT_BRANCH   := $(shell git symbolic-ref --short HEAD)

ifeq ($(strip $(shell git status --porcelain 2>/dev/null)),)
export ATMOSPHERE_GIT_REVISION := $(ATMOSPHERE_GIT_BRANCH)-$(shell git rev-parse --short HEAD)
else
export ATMOSPHERE_GIT_REVISION := $(ATMOSPHERE_GIT_BRANCH)-$(shell git rev-parse --short HEAD)-dirty
endif

export ATMOSPHERE_GIT_HASH := $(shell git rev-parse --short=16 HEAD)

ATMOSPHERE_DEFINES += -DATMOSPHERE_GIT_BRANCH=\"$(ATMOSPHERE_GIT_BRANCH)\" -DATMOSPHERE_GIT_REVISION=\"$(ATMOSPHERE_GIT_REVISION)\" -DATMOSPHERE_GIT_HASH="0x$(ATMOSPHERE_GIT_HASH)"

#---------------------------------------------------------------------------------
# Ensure top directory is set.
#---------------------------------------------------------------------------------
TOPDIR ?= $(CURDIR)

#---------------------------------------------------------------------------------
# TARGET is the name of the output
# SOURCES is a list of directories containing source code
# DATA is a list of directories containing data files
# INCLUDES is a list of directories containing header files
#---------------------------------------------------------------------------------
TARGET       := $(notdir $(CURDIR))
BUILD        := build
DATA         := data
INCLUDES     := include

GENERAL_SOURCE_DIRS=$1 $(foreach d,$(filter-out $1/arch $1/board $1/os $1/cpu $1,$(wildcard $1/*)),$(if $(wildcard $d/.),$(call DIR_WILDCARD,$d) $d,))
SPECIFIC_SOURCE_DIRS=$(if $(wildcard $1/$2/$3/.*),$1/$2/$3 $(call DIR_WILDCARD,$1/$2/$3),$(if $(wildcard $1/$2/generic/.*), $1/$2/generic $(call DIR_WILDCARD,$1/$2/generic),))
UNFILTERED_SOURCE_DIRS=$1 $(foreach d,$(wildcard $1/*),$(if $(wildcard $d/.),$(call DIR_WILDCARD,$d) $d,))

ALL_SOURCE_DIRS=$(call GENERAL_SOURCE_DIRS,$1) \
                $(call SPECIFIC_SOURCE_DIRS,$1,arch,$(ATMOSPHERE_ARCH_DIR)) \
                $(call SPECIFIC_SOURCE_DIRS,$1,board,$(ATMOSPHERE_BOARD_DIR)) \
                $(call SPECIFIC_SOURCE_DIRS,$1,os,$(ATMOSPHERE_OS_DIR)) \
                $(call SPECIFIC_SOURCE_DIRS,$1,cpu,$(ATMOSPHERE_ARCH_DIR)/$(ATMOSPHERE_CPU_DIR))

SOURCES      ?= $(call ALL_SOURCE_DIRS,source)

FIND_SPECIFIC_SOURCE_FILES= $(notdir $(wildcard $1/*.$2.$3.$4)) $(filter-out $(subst .$2.$3.,.$2.generic.,$(notdir $(wildcard $1/*.$2.$3.$4))),$(notdir $(wildcard $1/*.$2.generic.$4)))

FIND_SPECIFIC_SOURCE_FILES_EX=$(foreach ext,$3,$(notdir $(wildcard $1/*.$2.$(ext).$4))) $(filter-out $(foreach ext,$3,$(subst .$2.$(ext).,.$2.generic.,$(notdir $(wildcard $1/*.$2.$(ext).$4)))),$(notdir $(wildcard $1/*.$2.generic.$4)))

FIND_SOURCE_FILES=$(foreach dir,$1,$(filter-out $(notdir $(wildcard $(dir)/*.arch.*.$2)) \
                                                    $(notdir $(wildcard $(dir)/*.board.*.$2)) \
                                                    $(notdir $(wildcard $(dir)/*.os.*.$2)) \
                                                    $(notdir $(wildcard $(dir)/.cpu.*.$2)), \
                                                $(notdir $(wildcard $(dir)/*.$2)))) \
                  $(foreach dir,$1,$(call FIND_SPECIFIC_SOURCE_FILES,$(dir),arch,$(ATMOSPHERE_ARCH_NAME),$2)) \
                  $(foreach dir,$1,$(call FIND_SPECIFIC_SOURCE_FILES,$(dir),board,$(ATMOSPHERE_BOARD_NAME),$2)) \
                  $(foreach dir,$1,$(call FIND_SPECIFIC_SOURCE_FILES,$(dir),os,$(ATMOSPHERE_OS_NAME),$2)) \
                  $(foreach dir,$1,$(call FIND_SPECIFIC_SOURCE_FILES_EX,$(dir),cpu,$(ATMOSPHERE_CPU_NAME) $(ATMOSPHERE_CPU_EXTENSIONS),$2))

ATMOSPHERE_GCH_IDENTIFIER ?= ams_placeholder_gch_identifier

#---------------------------------------------------------------------------------
# Rules for compiling pre-compiled headers
#---------------------------------------------------------------------------------
%.hpp.gch/$(ATMOSPHERE_GCH_IDENTIFIER): %.hpp | %.hpp.gch
	@echo Precompiling $(notdir $<) for $(ATMOSPHERE_GCH_IDENTIFIER)
	$(SILENTCMD)$(CXX) -w -x c++-header -MMD -MP -MQ$@ -MF $(DEPSDIR)/$(notdir $*).d $(CXXFLAGS) -c $< -o $@ $(ERROR_FILTER)

%.hpp.gch: %.hpp
	@echo Precompiling $(notdir $<)
	$(SILENTCMD)$(CXX) -w -x c++-header -MMD -MP -MQ$@ -MF $(DEPSDIR)/$(notdir $*).d $(CXXFLAGS) -c $< -o $@ $(ERROR_FILTER)
