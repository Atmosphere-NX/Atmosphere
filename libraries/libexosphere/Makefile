ATMOSPHERE_BUILD_CONFIGS :=
all: arm64-release arm-release

define ATMOSPHERE_ADD_TARGET

ATMOSPHERE_BUILD_CONFIGS += $(strip $1)

arm64-$(strip $1):
	@$$(MAKE) -f arm64.mk $(strip $1)

arm-$(strip $1):
	@$$(MAKE) -f arm.mk $(strip $1)

clean-arm64-$(strip $1):
	@$$(MAKE) -f arm64.mk clean-$(strip $1)

clean-arm-$(strip $1):
	@$$(MAKE) -f arm.mk clean-$(strip $1)

endef

$(eval $(call ATMOSPHERE_ADD_TARGET, release))
$(eval $(call ATMOSPHERE_ADD_TARGET, debug))
$(eval $(call ATMOSPHERE_ADD_TARGET, audit))

clean-arm64:
	@$(MAKE) -f arm64.mk clean

clean-arm:
	@$(MAKE) -f arm.mk clean

clean: clean-arm64 clean-arm

.PHONY: all clean clean-arm64 clean-arm $(foreach config,$(ATMOSPHERE_BUILD_CONFIGS),arm64-$(config) arm-$(config) clean-arm64-$(config) clean-arm-$(config))
