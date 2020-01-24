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
export ATMOSPHERE_CXXFLAGS := -fno-rtti -fno-exceptions -std=gnu++17
export ATMOSPHERE_ASFLAGS  :=


ifeq ($(ATMOSPHERE_BOARD),nx-hac-001)
export ATMOSPHERE_ARCH_DIR   := arch/arm64
export ATMOSPHERE_BOARD_DIR  := board/nintendo/switch
export ATMOSPHERE_OS_DIR     := os/horizon

export ATMOSPHERE_ARCH_NAME  := arm64
export ATMOSPHERE_BOARD_NAME := nintendo_switch
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
SOURCES      ?= source $(foreach d,$(filter-out source/arch source/board source,$(wildcard source)),$(call DIR_WILDCARD,$d) $d)

ifneq ($(strip $(wildcard source/$(ATMOSPHERE_ARCH_DIR)/.*)),)
SOURCES += source/$(ATMOSPHERE_ARCH_DIR) $(call DIR_WILDCARD,source/$(ATMOSPHERE_ARCH_DIR))
endif
ifneq ($(strip $(wildcard source/$(ATMOSPHERE_BOARD_DIR)/.*)),)
SOURCES += source/$(ATMOSPHERE_BOARD_DIR $(call DIR_WILDCARD,source/$(ATMOSPHERE_BOARD_DIR))
endif
ifneq ($(strip $(wildcard source/$(ATMOSPHERE_OS_DIR)/.*)),)
SOURCES += source/$(ATMOSPHERE_OS_DIR $(call DIR_WILDCARD,source/$(ATMOSPHERE_OS_DIR))
endif
