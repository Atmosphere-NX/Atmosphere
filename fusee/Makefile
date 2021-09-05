ATMOSPHERE_BUILD_CONFIGS :=
all: release

define ATMOSPHERE_ADD_TARGET

ATMOSPHERE_BUILD_CONFIGS += $(strip $1)

$(strip $1): fusee$(strip $2).bin

fusee$(strip $2).bin: loader_stub/loader_stub$(strip $2).bin
	@cp loader_stub/loader_stub$(strip $2).bin fusee$(strip $2).bin
	@echo "Built fusee$(strip $2).bin..."

check_program_$(strip $1):
	@$$(MAKE) -C program $(strip $1)

loader_stub/loader_stub$(strip $2).bin: check_program_$(strip $1)
	@$$(MAKE) -C loader_stub $(strip $1)

clean-$(strip $1): clean-program-$(strip $1) clean-loader_stub-$(strip $1)
	@rm -rf fusee$(strip $2).bin

clean-program-$(strip $1):
	@$$(MAKE) -C program clean-$(strip $1)

clean-loader_stub-$(strip $1):
	@$$(MAKE) -C loader_stub clean-$(strip $1)

endef

$(eval $(call ATMOSPHERE_ADD_TARGET, release, ))
$(eval $(call ATMOSPHERE_ADD_TARGET, debug, _debug))
$(eval $(call ATMOSPHERE_ADD_TARGET, audit, _audit))

clean: clean-program clean-loader_stub
	@rm -rf fusee*.bin package3*

clean-program:
	@$(MAKE) -C program clean

clean-loader_stub:
	@$(MAKE) -C loader_stub clean

.PHONY: all clean clean-program clean-loader_stub $(foreach config,$(ATMOSPHERE_BUILD_CONFIGS),check_program_$(config) check_warmboot_$(strip $1) clean-$(config) clean-program-$(config) clean-loader_stub-$(config) clean-warmboot-$(config))
