#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------
DIR_WILDCARD=$(foreach d,$(wildcard $(1:=/*)),$(if $(wildcard $d/.),$(call DIR_WILDCARD,$d) $d,))

export ATMOSPHERE_CONFIG_MAKE_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
export ATMOSPHERE_LIBRARIES_DIR   := $(ATMOSPHERE_CONFIG_MAKE_DIR)/..

ifeq ($(strip $(ATMOSPHERE_BOARD)),)
export ATMOSPHERE_BOARD := nx-hac-001

ifeq ($(strip $(ATMOSPHERE_CPU)),)
export ATMOSPHERE_CPU   := arm-cortex-a57
endif

endif

export ATMOSPHERE_DEFINES  := -DATMOSPHERE
export ATMOSPHERE_SETTINGS := -fPIE -g
export ATMOSPHERE_CFLAGS   := -Wall -ffunction-sections -fdata-sections -fno-strict-aliasing -fwrapv \
                          -fno-asynchronous-unwind-tables -fno-unwind-tables -fno-stack-protector
export ATMOSPHERE_CXXFLAGS := -fno-rtti -fno-exceptions -std=gnu++20
export ATMOSPHERE_ASFLAGS  :=


ifeq ($(ATMOSPHERE_BOARD),nx-hac-001)

ifeq ($(ATMOSPHERE_CPU),arm-cortex-a57)
export ATMOSPHERE_ARCH_DIR   := arch/arm64
export ATMOSPHERE_BOARD_DIR  := board/nintendo/nx
export ATMOSPHERE_OS_DIR     := os/horizon

export ATMOSPHERE_ARCH_NAME  := arm64
export ATMOSPHERE_BOARD_NAME := nintendo_nx
export ATMOSPHERE_OS_NAME    := horizon
else ifeq ($(ATMOSPHERE_CPU),arm7tdmi)
export ATMOSPHERE_ARCH_DIR   := arch/arm
export ATMOSPHERE_BOARD_DIR  := board/nintendo/nx_bpmp
export ATMOSPHERE_OS_DIR     := os/horizon

export ATMOSPHERE_ARCH_NAME  := arm
export ATMOSPHERE_BOARD_NAME := nintendo_nx
export ATMOSPHERE_OS_NAME    := horizon
endif

endif

ifeq ($(ATMOSPHERE_CPU),arm-cortex-a57)
export ATMOSPHERE_CPU_DIR    := arch/arm64/cpu/cortex_a57
export ATMOSPHERE_CPU_NAME   := arm_cortex_a57
endif

ifeq ($(ATMOSPHERE_CPU),arm7tdmi)
export ATMOSPHERE_CPU_DIR    := arch/arm/cpu/arm7tdmi
export ATMOSPHERE_CPU_NAME   := arm7tdmi
endif


export ATMOSPHERE_ARCH_MAKE_DIR  := $(ATMOSPHERE_CONFIG_MAKE_DIR)/$(ATMOSPHERE_ARCH_DIR)
export ATMOSPHERE_BOARD_MAKE_DIR := $(ATMOSPHERE_CONFIG_MAKE_DIR)/$(ATMOSPHERE_BOARD_DIR)
export ATMOSPHERE_OS_MAKE_DIR    := $(ATMOSPHERE_CONFIG_MAKE_DIR)/$(ATMOSPHERE_OS_DIR)
export ATMOSPHERE_CPU_MAKE_DIR   := $(ATMOSPHERE_CONFIG_MAKE_DIR)/$(ATMOSPHERE_CPU_DIR)

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

GENERAL_SOURCE_DIRS=$1 $(foreach d,$(filter-out $1/arch $1/board $1,$(wildcard $1/*)),$(if $(wildcard $d/.),$(call DIR_WILDCARD,$d) $d,))
SPECIFIC_SOURCE_DIRS=$(if $(wildcard $1/$2/.*),$1/$2 $(call DIR_WILDCARD,$1/$2),)
ALL_SOURCE_DIRS=$(call GENERAL_SOURCE_DIRS,$1) $(call SPECIFIC_SOURCE_DIRS,$1,$(ATMOSPHERE_ARCH_DIR)) $(call SPECIFIC_SOURCE_DIRS,$1,$(ATMOSPHERE_BOARD_DIR)) $(call SPECIFIC_SOURCE_DIRS,$1,$(ATMOSPHERE_OS_DIR))

SOURCES      ?= $(call ALL_SOURCE_DIRS,source)

#---------------------------------------------------------------------------------
# Rules for compiling pre-compiled headers
#---------------------------------------------------------------------------------
%.gch: %.hpp
	@echo $<
	$(CXX) -w -x c++-header -MMD -MP -MF $(DEPSDIR)/$*.d $(CXXFLAGS) -c $< -o $@ $(ERROR_FILTER)
	@cp $@ $(<).gch
