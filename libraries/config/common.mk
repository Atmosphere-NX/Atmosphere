#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------

export ATMOSPHERE_CONFIG_MAKE_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
export ATMOSPHERE_LIBRARIES_DIR   := $(ATMOSPHERE_CONFIG_MAKE_DIR)/..

ifeq ($(strip $(ATMOSPHERE_BOARD)),)
export ATMOSPHERE_BOARD := nx-hac-001
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


export ATMOSPHERE_ARCH_MAKE_DIR  := $(ATMOSPHERE_CONFIG_MAKE_DIR)/$(ATMOSPHERE_ARCH_DIR)
export ATMOSPHERE_BOARD_MAKE_DIR := $(ATMOSPHERE_CONFIG_MAKE_DIR)/$(ATMOSPHERE_BOARD_DIR)
export ATMOSPHERE_OS_MAKE_DIR    := $(ATMOSPHERE_CONFIG_MAKE_DIR)/$(ATMOSPHERE_OS_DIR)

include $(ATMOSPHERE_ARCH_MAKE_DIR)/arch.mk
include $(ATMOSPHERE_BOARD_MAKE_DIR)/board.mk
include $(ATMOSPHERE_OS_MAKE_DIR)/os.mk

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
# TARGET is the name of the output
# SOURCES is a list of directories containing source code
# DATA is a list of directories containing data files
# INCLUDES is a list of directories containing header files
#---------------------------------------------------------------------------------
export TARGET       :=	$(notdir $(CURDIR))
export BUILD        :=  build
export DATA         :=	data
export INCLUDES     :=  include
export SOURCES      ?=	$(shell find source -type d \
                          -not \( -path source/arch -prune \)  \
                          -not \( -path source/board -prune \) \)

ifneq ($(strip $(wildcard source/$(ATMOSPHERE_ARCH_DIR)./.*)),)
SOURCES += $(shell find source/$(ATMOSPHERE_ARCH_DIR)  -type d)
endif
ifneq ($(strip $(wildcard source/$(ATMOSPHERE_BOARD_DIR)./.*)),)
SOURCES += $(shell find source/$(ATMOSPHERE_BOARD_DIR) -type d)
endif
ifneq ($(strip $(wildcard source/$(ATMOSPHERE_OS_DIR)./.*)),)
SOURCES += $(shell find source/$(ATMOSPHERE_OS_DIR) -type d)
endif
