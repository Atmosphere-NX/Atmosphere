ATMOSPHERE_BUILD_CONFIGS :=
all: release

define ATMOSPHERE_ADD_TARGET

ATMOSPHERE_BUILD_CONFIGS += $(strip $1)

$(strip $1): exosphere$(strip $2).bin warmboot$(strip $2).bin mariko_fatal$(strip $2).bin

exosphere$(strip $2).bin: loader_stub/loader_stub$(strip $2).bin
	@cp loader_stub/loader_stub$(strip $2).bin exosphere$(strip $2).bin
	@printf LENY >> exosphere$(strip $2).bin
	@echo "Built exosphere$(strip $2).bin..."

warmboot$(strip $2).bin: warmboot/warmboot$(strip $2).bin
	@cp warmboot/warmboot$(strip $2).bin warmboot$(strip $2).bin
	@echo "Built warmboot$(strip $2).bin..."

mariko_fatal$(strip $2).bin: mariko_fatal/mariko_fatal$(strip $2).bin
	@cp mariko_fatal/mariko_fatal$(strip $2).bin mariko_fatal$(strip $2).bin
	@echo "Built mariko_fatal$(strip $2).bin..."

check_program_$(strip $1):
	@$$(MAKE) -C program $(strip $1)

check_warmboot_$(strip $1):
	@$$(MAKE) -C warmboot $(strip $1)

check_mariko_fatal_$(strip $1):
	@$$(MAKE) -C mariko_fatal $(strip $1)

loader_stub/loader_stub$(strip $2).bin: check_program_$(strip $1)
	@$$(MAKE) -C loader_stub $(strip $1)

warmboot/warmboot$(strip $2).bin: check_warmboot_$(strip $1)

mariko_fatal/mariko_fatal$(strip $2).bin: check_mariko_fatal_$(strip $1)

clean-$(strip $1): clean-program-$(strip $1) clean-loader_stub-$(strip $1) clean-warmboot-$(strip $1) clean-mariko_fatal-$(strip $1)
	@rm -rf exosphere$(strip $2).bin warmboot$(strip $2).bin mariko_fatal$(strip $2).bin

clean-program-$(strip $1):
	@$$(MAKE) -C program clean-$(strip $1)

clean-loader_stub-$(strip $1):
	@$$(MAKE) -C loader_stub clean-$(strip $1)

clean-warmboot-$(strip $1):
	@$$(MAKE) -C warmboot clean-$(strip $1)

clean-mariko_fatal-$(strip $1):
	@$$(MAKE) -C mariko_fatal clean-$(strip $1)

endef

$(eval $(call ATMOSPHERE_ADD_TARGET, release, ))
$(eval $(call ATMOSPHERE_ADD_TARGET, debug, _debug))
$(eval $(call ATMOSPHERE_ADD_TARGET, audit, _audit))

clean: clean-program clean-loader_stub clean-warmboot
	@rm -rf exosphere*.bin warmboot*.bin mariko_fatal*.bin

clean-program:
	@$(MAKE) -C program clean

clean-loader_stub:
	@$(MAKE) -C loader_stub clean

clean-warmboot:
	@$(MAKE) -C warmboot clean

clean-mariko_fatal:
	@$(MAKE) -C mariko_fatal clean

.PHONY: all clean clean-program clean-loader_stub clean-warmboot $(foreach config,$(ATMOSPHERE_BUILD_CONFIGS),check_program_$(config) check_warmboot_$(strip $1) clean-$(config) clean-program-$(config) clean-loader_stub-$(config) clean-warmboot-$(config))
