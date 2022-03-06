ATMOSPHERE_BUILD_CONFIGS :=
all: nx_release

THIS_MAKEFILE     := $(abspath $(lastword $(MAKEFILE_LIST)))
CURRENT_DIRECTORY := $(abspath $(dir $(THIS_MAKEFILE)))

define ATMOSPHERE_ADD_TARGET

ATMOSPHERE_BUILD_CONFIGS += $(strip $1)

$(strip $1):
	@echo "Building $(strip $1)"
	@$$(MAKE) -f $(CURRENT_DIRECTORY)/unit_test.mk ATMOSPHERE_MAKEFILE_TARGET="$(strip $1)" ATMOSPHERE_BUILD_NAME="$(strip $2)" ATMOSPHERE_BOARD="$(strip $3)" ATMOSPHERE_CPU="$(strip $4)" $(strip $5)

clean-$(strip $1):
	@echo "Cleaning $(strip $1)"
	@$$(MAKE) -f $(CURRENT_DIRECTORY)/unit_test.mk clean ATMOSPHERE_MAKEFILE_TARGET="$(strip $1)" ATMOSPHERE_BUILD_NAME="$(strip $2)" ATMOSPHERE_BOARD="$(strip $3)" ATMOSPHERE_CPU="$(strip $4)" $(strip $5)

endef

define ATMOSPHERE_ADD_TARGETS

$(eval $(call ATMOSPHERE_ADD_TARGET, $(strip $1)_release, $(strip $2)release, $(strip $3), $(strip $4), \
    ATMOSPHERE_BUILD_SETTINGS="$(strip $5)" $(strip $6) \
))

$(eval $(call ATMOSPHERE_ADD_TARGET, $(strip $1)_debug, $(strip $2)debug, $(strip $3), $(strip $4), \
    ATMOSPHERE_BUILD_SETTINGS="$(strip $5) -DAMS_BUILD_FOR_DEBUGGING" ATMOSPHERE_BUILD_FOR_DEBUGGING=1 $(strip $6) \
))

$(eval $(call ATMOSPHERE_ADD_TARGET, $(strip $1)_audit, $(strip $2)audit, $(strip $3), $(strip $4), \
    ATMOSPHERE_BUILD_SETTINGS="$(strip $5) -DAMS_BUILD_FOR_AUDITING" ATMOSPHERE_BUILD_FOR_DEBUGGING=1 ATMOSPHERE_BUILD_FOR_AUDITING=1 $(strip $6) \
))

endef


$(eval $(call ATMOSPHERE_ADD_TARGETS, nx,                      , nx-hac-001, arm-cortex-a57,,))

$(eval $(call ATMOSPHERE_ADD_TARGETS, win_x64,                 , generic_windows, generic_x64,,))

$(eval $(call ATMOSPHERE_ADD_TARGETS, linux_x64,               , generic_linux, generic_x64,,))
$(eval $(call ATMOSPHERE_ADD_TARGETS, linux_x64_clang,   clang_, generic_linux, generic_x64,, ATMOSPHERE_COMPILER_NAME="clang"))
$(eval $(call ATMOSPHERE_ADD_TARGETS, linux_arm64_clang, clang_, generic_linux, generic_arm64,, ATMOSPHERE_COMPILER_NAME="clang"))

$(eval $(call ATMOSPHERE_ADD_TARGETS, macos_x64,               , generic_macos, generic_x64,,))
$(eval $(call ATMOSPHERE_ADD_TARGETS, macos_arm64,             , generic_macos, generic_arm64,,))

clean: $(foreach config,$(ATMOSPHERE_BUILD_CONFIGS),clean-$(config))

.PHONY: all clean $(foreach config,$(ATMOSPHERE_BUILD_CONFIGS), $(config) clean-$(config))
