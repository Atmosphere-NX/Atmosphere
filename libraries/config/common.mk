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

ifeq ($(ATMOSPHERE_BUILD_NAME),)
export ATMOSPHERE_BUILD_NAME := release
endif

ifeq ($(strip $(ATMOSPHERE_COMPILER_NAME)),)

ifneq ($(strip $(ATMOSPHERE_BOARD)),generic_macos)
export ATMOSPHERE_COMPILER_NAME := gcc
else
export ATMOSPHERE_COMPILER_NAME := clang
endif

export ATMOSPHERE_BUILD_NAME := release
endif

ATMOSPHERE_BUILD_SETTINGS ?=

export ATMOSPHERE_DEFINES  := -DATMOSPHERE
export ATMOSPHERE_SETTINGS := -fPIE -g $(ATMOSPHERE_BUILD_SETTINGS)
export ATMOSPHERE_CFLAGS   := -Wall -ffunction-sections -fdata-sections -fno-strict-aliasing -fwrapv  \
                              -fno-asynchronous-unwind-tables -fno-unwind-tables -fno-stack-protector \
                              -Wno-format-zero-length

ifeq ($(strip $(ATMOSPHERE_COMPILER_NAME)),gcc)
export ATMOSPHERE_CFLAGS += -Wno-stringop-truncation -Wno-format-truncation
else ifeq ($(strip $(ATMOSPHERE_COMPILER_NAME)),clang)
export ATMOSPHERE_CFLAGS += -Wno-c99-designator -Wno-gnu-alignof-expression -Wno-unused-private-field
endif

export ATMOSPHERE_CXXFLAGS := -fno-rtti -fno-exceptions -std=gnu++23 -Wno-invalid-offsetof
export ATMOSPHERE_ASFLAGS  :=


ifeq ($(ATMOSPHERE_BOARD),nx-hac-001)

ifeq ($(ATMOSPHERE_CPU),arm-cortex-a57)
export ATMOSPHERE_ARCH_DIR   := arm64
export ATMOSPHERE_BOARD_DIR  := nintendo/nx
export ATMOSPHERE_OS_DIR     := horizon

export ATMOSPHERE_ARCH_NAME  := arm64
export ATMOSPHERE_BOARD_NAME := nintendo_nx
export ATMOSPHERE_OS_NAME    := horizon

export ATMOSPHERE_SUB_ARCH_DIR  := armv8a
export ATMOSPHERE_SUB_ARCH_NAME := armv8a

export ATMOSPHERE_CPU_EXTENSIONS := arm_crypto_extension aarch64_crypto_extension

export ATMOSPHERE_BOOT_CPU := arm7tdmi
export ATMOSPHERE_BOOT_ARCH_NAME     := arm
export ATMOSPHERE_BOOT_BOARD_NAME    := nintendo_nx
export ATMOSPHERE_BOOT_OS_NAME       := horizon
export ATMOSPHERE_BOOT_SUB_ARCH_NAME := armv4t
else ifeq ($(ATMOSPHERE_CPU),arm7tdmi)
export ATMOSPHERE_ARCH_DIR   := arm
export ATMOSPHERE_BOARD_DIR  := nintendo/nx_bpmp
export ATMOSPHERE_OS_DIR     := horizon

export ATMOSPHERE_ARCH_NAME  := arm
export ATMOSPHERE_BOARD_NAME := nintendo_nx
export ATMOSPHERE_OS_NAME    := horizon

export ATMOSPHERE_SUB_ARCH_DIR  := armv4t
export ATMOSPHERE_SUB_ARCH_NAME := armv4t

export ATMOSPHERE_CPU_EXTENSIONS :=
endif

export ATMOSPHERE_LIBDIRS :=

else ifeq ($(ATMOSPHERE_BOARD),qemu-virt)


ifeq ($(ATMOSPHERE_CPU),arm-cortex-a57)
export ATMOSPHERE_ARCH_DIR   := arm64
export ATMOSPHERE_BOARD_DIR  := qemu/virt
export ATMOSPHERE_OS_DIR     := horizon

export ATMOSPHERE_ARCH_NAME  := arm64
export ATMOSPHERE_BOARD_NAME := qemu_virt
export ATMOSPHERE_OS_NAME    := horizon

export ATMOSPHERE_SUB_ARCH_DIR  = armv8a
export ATMOSPHERE_SUB_ARCH_NAME = armv8a

export ATMOSPHERE_CPU_EXTENSIONS := arm_crypto_extension aarch64_crypto_extension
endif

export ATMOSPHERE_LIBDIRS :=

else ifeq ($(ATMOSPHERE_BOARD),generic_windows)

ifeq ($(ATMOSPHERE_CPU),generic_x64)
export ATMOSPHERE_ARCH_DIR   := x64
export ATMOSPHERE_BOARD_DIR  := generic/windows
export ATMOSPHERE_OS_DIR     := windows

export ATMOSPHERE_ARCH_NAME  := x64
export ATMOSPHERE_BOARD_NAME := generic_windows
export ATMOSPHERE_OS_NAME    := windows

endif

else ifeq ($(ATMOSPHERE_BOARD),generic_linux)

ifeq ($(ATMOSPHERE_CPU),generic_x64)
export ATMOSPHERE_ARCH_DIR   := x64
export ATMOSPHERE_ARCH_NAME  := x64
else ifeq ($(ATMOSPHERE_CPU),generic_arm64)
export ATMOSPHERE_ARCH_DIR   := arm64
export ATMOSPHERE_ARCH_NAME  := arm64
endif

export ATMOSPHERE_BOARD_DIR  := generic/linux
export ATMOSPHERE_OS_DIR     := linux

export ATMOSPHERE_BOARD_NAME := generic_linux
export ATMOSPHERE_OS_NAME    := linux

else ifeq ($(ATMOSPHERE_BOARD),generic_macos)

ifeq ($(ATMOSPHERE_CPU),generic_x64)
export ATMOSPHERE_ARCH_DIR   := x64
export ATMOSPHERE_ARCH_NAME  := x64
else ifeq ($(ATMOSPHERE_CPU),generic_arm64)
export ATMOSPHERE_ARCH_DIR   := arm64
export ATMOSPHERE_ARCH_NAME  := arm64
endif

export ATMOSPHERE_BOARD_DIR  := generic/macos
export ATMOSPHERE_OS_DIR     := macos

export ATMOSPHERE_BOARD_NAME := generic_macos
export ATMOSPHERE_OS_NAME    := macos

endif

ifeq ($(ATMOSPHERE_CPU),arm-cortex-a57)
export ATMOSPHERE_CPU_DIR    := cortex_a57
export ATMOSPHERE_CPU_NAME   := arm_cortex_a57
endif

ifeq ($(ATMOSPHERE_CPU),arm7tdmi)
export ATMOSPHERE_CPU_DIR    := arm7tdmi
export ATMOSPHERE_CPU_NAME   := arm7tdmi
endif

ifeq ($(ATMOSPHERE_CPU),generic_x64)
export ATMOSPHERE_CPU_DIR    := generic_x64
export ATMOSPHERE_CPU_NAME   := generic_x64
endif

ifeq ($(ATMOSPHERE_CPU),generic_arm64)
export ATMOSPHERE_CPU_DIR    := generic_arm64
export ATMOSPHERE_CPU_NAME   := generic_arm64
endif

export ATMOSPHERE_ARCH_MAKE_DIR  := $(ATMOSPHERE_CONFIG_MAKE_DIR)/arch/$(ATMOSPHERE_ARCH_DIR)
export ATMOSPHERE_BOARD_MAKE_DIR := $(ATMOSPHERE_CONFIG_MAKE_DIR)/board/$(ATMOSPHERE_BOARD_DIR)
export ATMOSPHERE_OS_MAKE_DIR    := $(ATMOSPHERE_CONFIG_MAKE_DIR)/os/$(ATMOSPHERE_OS_DIR)
export ATMOSPHERE_CPU_MAKE_DIR   := $(ATMOSPHERE_ARCH_MAKE_DIR)/cpu/$(ATMOSPHERE_CPU_DIR)

ifneq ($(strip $(ATMOSPHERE_SUB_ARCH_NAME)),)
export ATMOSPHERE_FULL_NAME   := $(ATMOSPHERE_BOARD_NAME)_$(ATMOSPHERE_ARCH_NAME)_$(ATMOSPHERE_SUB_ARCH_NAME)_$(ATMOSPHERE_BUILD_NAME)
export ATMOSPHERE_LIBRARY_DIR := lib/$(ATMOSPHERE_BOARD_NAME)_$(ATMOSPHERE_ARCH_NAME)_$(ATMOSPHERE_SUB_ARCH_NAME)/$(ATMOSPHERE_BUILD_NAME)
export ATMOSPHERE_BUILD_DIR   := build/$(ATMOSPHERE_BOARD_NAME)_$(ATMOSPHERE_ARCH_NAME)_$(ATMOSPHERE_SUB_ARCH_NAME)/$(ATMOSPHERE_BUILD_NAME)
export ATMOSPHERE_OUT_DIR     := out/$(ATMOSPHERE_BOARD_NAME)_$(ATMOSPHERE_ARCH_NAME)_$(ATMOSPHERE_SUB_ARCH_NAME)/$(ATMOSPHERE_BUILD_NAME)
else
export ATMOSPHERE_FULL_NAME   := $(ATMOSPHERE_BOARD_NAME)_$(ATMOSPHERE_ARCH_NAME)_$(ATMOSPHERE_BUILD_NAME)
export ATMOSPHERE_LIBRARY_DIR := lib/$(ATMOSPHERE_BOARD_NAME)_$(ATMOSPHERE_ARCH_NAME)/$(ATMOSPHERE_BUILD_NAME)
export ATMOSPHERE_BUILD_DIR   := build/$(ATMOSPHERE_BOARD_NAME)_$(ATMOSPHERE_ARCH_NAME)/$(ATMOSPHERE_BUILD_NAME)
export ATMOSPHERE_OUT_DIR     := out/$(ATMOSPHERE_BOARD_NAME)_$(ATMOSPHERE_ARCH_NAME)/$(ATMOSPHERE_BUILD_NAME)
endif

ifneq ($(strip $(ATMOSPHERE_BOOT_ARCH_NAME)),)

ifneq ($(strip $(ATMOSPHERE_SUB_ARCH_NAME)),)
export ATMOSPHERE_BOOT_FULL_NAME   := $(ATMOSPHERE_BOOT_BOARD_NAME)_$(ATMOSPHERE_BOOT_ARCH_NAME)_$(ATMOSPHERE_BOOT_SUB_ARCH_NAME)_$(ATMOSPHERE_BUILD_NAME)
export ATMOSPHERE_BOOT_LIBRARY_DIR := lib/$(ATMOSPHERE_BOOT_BOARD_NAME)_$(ATMOSPHERE_BOOT_ARCH_NAME)_$(ATMOSPHERE_BOOT_SUB_ARCH_NAME)/$(ATMOSPHERE_BUILD_NAME)
export ATMOSPHERE_BOOT_BUILD_DIR   := build/$(ATMOSPHERE_BOOT_BOARD_NAME)_$(ATMOSPHERE_BOOT_ARCH_NAME)_$(ATMOSPHERE_BOOT_SUB_ARCH_NAME)/$(ATMOSPHERE_BUILD_NAME)
export ATMOSPHERE_BOOT_OUT_DIR     := out/$(ATMOSPHERE_BOOT_BOARD_NAME)_$(ATMOSPHERE_BOOT_ARCH_NAME)_$(ATMOSPHERE_BOOT_SUB_ARCH_NAME)/$(ATMOSPHERE_BUILD_NAME)
else
export ATMOSPHERE_BOOT_FULL_NAME   := $(ATMOSPHERE_BOOT_BOARD_NAME)_$(ATMOSPHERE_BOOT_ARCH_NAME)_$(ATMOSPHERE_BUILD_NAME)
export ATMOSPHERE_BOOT_LIBRARY_DIR := lib/$(ATMOSPHERE_BOOT_BOARD_NAME)_$(ATMOSPHERE_BOOT_ARCH_NAME)/$(ATMOSPHERE_BUILD_NAME)
export ATMOSPHERE_BOOT_BUILD_DIR   := build/$(ATMOSPHERE_BOOT_BOARD_NAME)_$(ATMOSPHERE_BOOT_ARCH_NAME)/$(ATMOSPHERE_BUILD_NAME)
export ATMOSPHERE_BOOT_OUT_DIR     := out/$(ATMOSPHERE_BOOT_BOARD_NAME)_$(ATMOSPHERE_BOOT_ARCH_NAME)/$(ATMOSPHERE_BUILD_NAME)
endif

else
export ATMOSPHERE_BOOT_FULL_NAME   := $(ATMOSPHERE_FULL_NAME)
export ATMOSPHERE_BOOT_LIBRARY_DIR := $(ATMOSPHERE_LIBRARY_DIR)
export ATMOSPHERE_BOOT_BUILD_DIR   := $(ATMOSPHERE_BUILD_DIR)
export ATMOSPHERE_BOOT_OUT_DIR     := $(ATMOSPHERE_OUT_DIR)
endif

ifeq ($(strip $(ATMOSPHERE_BOOT_CPU)),)
export ATMOSPHERE_BOOT_CPU := $(ATMOSPHERE_CPU)
endif

include $(ATMOSPHERE_ARCH_MAKE_DIR)/arch.mk
include $(ATMOSPHERE_BOARD_MAKE_DIR)/board.mk
include $(ATMOSPHERE_OS_MAKE_DIR)/os.mk
include $(ATMOSPHERE_CPU_MAKE_DIR)/cpu.mk


ifneq ($(strip $(ATMOSPHERE_SUB_ARCH_NAME)),)
export ATMOSPHERE_SUB_ARCH_MAKE_DIR  := $(ATMOSPHERE_CONFIG_MAKE_DIR)/arch/$(ATMOSPHERE_SUB_ARCH_DIR)

include $(ATMOSPHERE_SUB_ARCH_MAKE_DIR)/arch.mk
endif


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

GENERAL_SOURCE_DIRS=$1 $(foreach d,$(filter-out $1/arch $1/board $1/os $1/cpu $1,$(wildcard $1/*)),$(if $(wildcard $d/.),$(filter-out $d,$(call GENERAL_SOURCE_DIRS,$d)) $d,))

SPECIFIC_SOURCE_DIRS=$(if $(wildcard $1/$2/$3/.*),$1/$2/$3 $(call DIR_WILDCARD,$1/$2/$3),$(if $(wildcard $1/$2/generic/.*), $1/$2/generic $(call DIR_WILDCARD,$1/$2/generic),))

ifneq ($(strip $(ATMOSPHERE_SUB_ARCH_NAME)),)
SPECIFIC_SOURCE_DIRS_ARCH=$(if $(wildcard $1/$2/$3/.*),$1/$2/$3 $(call DIR_WILDCARD,$1/$2/$3),$(if $(wildcard $1/$2/$4/.*),$1/$2/$4 $(call DIR_WILDCARD,$1/$2/$4),$(if $(wildcard $1/$2/generic/.*), $1/$2/generic $(call DIR_WILDCARD,$1/$2/generic),)))
else
SPECIFIC_SOURCE_DIRS_ARCH=$(if $(wildcard $1/$2/$3/.*),$1/$2/$3 $(call DIR_WILDCARD,$1/$2/$3),$(if $(wildcard $1/$2/generic/.*), $1/$2/generic $(call DIR_WILDCARD,$1/$2/generic),))
endif

UNFILTERED_SOURCE_DIRS=$1 $(foreach d,$(wildcard $1/*),$(if $(wildcard $d/.),$(call DIR_WILDCARD,$d) $d,))

ALL_SOURCE_DIRS=$(foreach d,$(call GENERAL_SOURCE_DIRS,$1), \
                    $d \
                    $(call SPECIFIC_SOURCE_DIRS_ARCH,$d,arch,$(ATMOSPHERE_ARCH_DIR),$(ATMOSPHERE_SUB_ARCH_DIR)) \
                    $(call SPECIFIC_SOURCE_DIRS,$d,board,$(ATMOSPHERE_BOARD_DIR)) \
                    $(call SPECIFIC_SOURCE_DIRS,$d,os,$(ATMOSPHERE_OS_DIR)) \
                    $(call SPECIFIC_SOURCE_DIRS,$d,cpu,$(ATMOSPHERE_ARCH_DIR)/$(ATMOSPHERE_CPU_DIR)) \
                )

SOURCES      ?= $(call ALL_SOURCE_DIRS,source)

FIND_SPECIFIC_SOURCE_FILES= $(notdir $(wildcard $1/*.$2.$3.$4)) $(filter-out $(subst .$2.$3.,.$2.generic.,$(notdir $(wildcard $1/*.$2.$3.$4))),$(notdir $(wildcard $1/*.$2.generic.$4)))

FIND_SPECIFIC_SOURCE_FILES_EX=$(foreach ext,$3,$(notdir $(wildcard $1/*.$2.$(ext).$4))) $(filter-out $(foreach ext,$3,$(subst .$2.$(ext).,.$2.generic.,$(notdir $(wildcard $1/*.$2.$(ext).$4)))),$(notdir $(wildcard $1/*.$2.generic.$4)))

FIND_SOURCE_FILES=$(foreach dir,$1,$(filter-out $(notdir $(wildcard $(dir)/*.arch.*.$2)) \
                                                    $(notdir $(wildcard $(dir)/*.board.*.$2)) \
                                                    $(notdir $(wildcard $(dir)/*.os.*.$2)) \
                                                    $(notdir $(wildcard $(dir)/.cpu.*.$2)), \
                                                $(notdir $(wildcard $(dir)/*.$2)))) \
                  $(foreach dir,$1,$(call FIND_SPECIFIC_SOURCE_FILES_EX,$(dir),arch,$(ATMOSPHERE_ARCH_NAME) $(ATMOSPHERE_SUB_ARCH_NAME),$2)) \
                  $(foreach dir,$1,$(call FIND_SPECIFIC_SOURCE_FILES,$(dir),board,$(ATMOSPHERE_BOARD_NAME),$2)) \
                  $(foreach dir,$1,$(call FIND_SPECIFIC_SOURCE_FILES,$(dir),os,$(ATMOSPHERE_OS_NAME),$2)) \
                  $(foreach dir,$1,$(call FIND_SPECIFIC_SOURCE_FILES_EX,$(dir),cpu,$(ATMOSPHERE_CPU_NAME) $(ATMOSPHERE_CPU_EXTENSIONS),$2))

ATMOSPHERE_GCH_IDENTIFIER := $(ATMOSPHERE_FULL_NAME)

#---------------------------------------------------------------------------------
# Python.  The scripts should work with Python 2 or 3, but 2 is preferred.
#---------------------------------------------------------------------------------
PYTHON = $(shell command -v python >/dev/null && echo python || echo python3)

#---------------------------------------------------------------------------------
# Export MAKE:
# GCC's LTO driver invokes Make internally.  This invocation respects both $(MAKE)
# and $(MAKEFLAGS), but only if they're in the environment.  By default, MAKEFLAGS
# is in the environment while MAKE isn't, so GCC will always use the default
# `make` command, yet it inherits MAKEFLAGS from the copy of Make the user
# invoked, which might have incompatible flags.  In practice this is an issue on
# macOS when running e.g. `gmake -j32 -Otarget`.  This behavior is arguably a bug
# in GCC and/or Make, but we can work around it by exporting MAKE.
#---------------------------------------------------------------------------------
export MAKE

#---------------------------------------------------------------------------------
# Rules for compiling pre-compiled headers
#---------------------------------------------------------------------------------
%.hpp.gch/$(ATMOSPHERE_GCH_IDENTIFIER): %.hpp %.hpp.gch
	@echo Precompiling $(notdir $<) for $(ATMOSPHERE_GCH_IDENTIFIER)
	$(SILENTCMD)$(CXX) -w -x c++-header -MMD -MP -MQ$@ -MF $(DEPSDIR)/$(notdir $*).d $(CXXFLAGS) -c $< -o $@ $(ERROR_FILTER)

%.hpp.gch: %.hpp
	@[ -d $@ ] || mkdir -p $@
