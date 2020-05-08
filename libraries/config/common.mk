#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------
DIR_WILDCARD=$(foreach d,$(wildcard $(1:=/*)),$(if $(wildcard $d/.),$(call DIR_WILDCARD,$d) $d,))

export ATMOSPHERE_CONFIG_MAKE_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
export ATMOSPHERE_LIBRARIES_DIR   := $(ATMOSPHERE_CONFIG_MAKE_DIR)/..

ifeq ($(strip $(ATMOSPHERE_BOARD)),)
export ATMOSPHERE_BOARD := nx-hac-001
endif

ifeq ($(strip $(ATMOSPHERE_CPU)),)
export ATMOSPHERE_CPU   := arm-cortex-a57
endif

export ATMOSPHERE_DEFINES  := -DATMOSPHERE
export ATMOSPHERE_SETTINGS := -fPIE -g
export ATMOSPHERE_CFLAGS   := -Wall -ffunction-sections -fdata-sections -fno-strict-aliasing -fwrapv \
                          -fno-asynchronous-unwind-tables -fno-unwind-tables -fno-stack-protector
export ATMOSPHERE_CXXFLAGS := -fno-rtti -fno-exceptions -std=gnu++20
export ATMOSPHERE_ASFLAGS  :=


ifeq ($(ATMOSPHERE_BOARD),nx-hac-001)
export ATMOSPHERE_ARCH_DIR   := arch/arm64
export ATMOSPHERE_BOARD_DIR  := board/nintendo/nx
export ATMOSPHERE_OS_DIR     := os/horizon

export ATMOSPHERE_ARCH_NAME  := arm64
export ATMOSPHERE_BOARD_NAME := nintendo_nx
export ATMOSPHERE_OS_NAME    := horizon
endif

ifeq ($(ATMOSPHERE_CPU),arm-cortex-a57)
export ATMOSPHERE_CPU_DIR    := arch/arm64/cpu/cortex_a57
export ATMOSPHERE_CPU_NAME   := arm_cortex_a57
endif


export ATMOSPHERE_ARCH_MAKE_DIR  := $(ATMOSPHERE_CONFIG_MAKE_DIR)/$(ATMOSPHERE_ARCH_DIR)
export ATMOSPHERE_BOARD_MAKE_DIR := $(ATMOSPHERE_CONFIG_MAKE_DIR)/$(ATMOSPHERE_BOARD_DIR)
export ATMOSPHERE_OS_MAKE_DIR    := $(ATMOSPHERE_CONFIG_MAKE_DIR)/$(ATMOSPHERE_OS_DIR)
export ATMOSPHERE_CPU_MAKE_DIR   := $(ATMOSPHERE_CONFIG_MAKE_DIR)/$(ATMOSPHERE_CPU_DIR)

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

ATMOSPHERE_DEFINES += -DATMOSPHERE_GIT_BRANCH=\"$(ATMOSPHERE_GIT_BRANCH)\" -DATMOSPHERE_GIT_REVISION=\"$(ATMOSPHERE_GIT_REVISION)\"

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
