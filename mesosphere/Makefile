ATMOSPHERE_BUILD_CONFIGS :=
all: release

define ATMOSPHERE_ADD_TARGET

ATMOSPHERE_BUILD_CONFIGS += $(strip $1)

$(strip $1): mesosphere$(strip $2).bin

mesosphere$(strip $2).bin: kernel/kernel$(strip $2).bin kernel_ldr/kernel_ldr$(strip $2).bin
	@python build_mesosphere.py kernel_ldr/kernel_ldr$(strip $2).bin kernel/kernel$(strip $2).bin mesosphere$(strip $2).bin $(4)
	@echo "Built mesosphere$(strip $2).bin..."

kernel/kernel$(strip $2).bin: check_libmeso$(strip $1)
	@$$(MAKE) -C kernel $(strip $1) $(3)

kernel_ldr/kernel_ldr$(strip $2).bin: check_libmeso$(strip $1)
	@$$(MAKE) -C kernel_ldr $(strip $1) $(3)

check_libmeso$(strip $1):
	@$$(MAKE) -C ../libraries/libmesosphere $(strip $1) $(3)

clean-$(strip $1):
	@$$(MAKE) -C ../libraries/libmesosphere clean-$(strip $1) $(3)
	@$$(MAKE) -C kernel clean-$(strip $1) $(3)
	@$$(MAKE) -C kernel_ldr clean-$(strip $1) $(3)
	@rm -f mesosphere$(strip $2).bin

endef

$(eval $(call ATMOSPHERE_ADD_TARGET, release,      ,,))
$(eval $(call ATMOSPHERE_ADD_TARGET, debug,  _debug,,))
$(eval $(call ATMOSPHERE_ADD_TARGET, audit,  _audit,,))

$(eval $(call ATMOSPHERE_ADD_TARGET, qemu_virt_release, _qemu_virt, \
	ATMOSPHERE_BOARD="qemu-virt" \
    ATMOSPHERE_CPU="arm-cortex-a57" \
    , ../tests/TestSvc/TestSvc.kip \
))

$(eval $(call ATMOSPHERE_ADD_TARGET, qemu_virt_debug, _qemu_virt_debug, \
	ATMOSPHERE_BOARD="qemu-virt" \
    ATMOSPHERE_CPU="arm-cortex-a57" \
    , ../tests/TestSvc/TestSvc.kip \
))

$(eval $(call ATMOSPHERE_ADD_TARGET, qemu_virt_audit, _qemu_virt_audit, \
	ATMOSPHERE_BOARD="qemu-virt" \
    ATMOSPHERE_CPU="arm-cortex-a57" \
    , ../tests/TestSvc/TestSvc.kip \
))

clean:
	@$(MAKE) -C ../libraries/libmesosphere clean
	@$(MAKE) -C kernel clean
	@$(MAKE) -C kernel_ldr clean
	@rm -f mesosphere*.bin

.PHONY: all clean $(foreach config,$(ATMOSPHERE_BUILD_CONFIGS),$(config) clean-$(config))
