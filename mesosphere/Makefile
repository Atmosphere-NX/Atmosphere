TARGETS := kernel.bin kernel_ldr.bin
CLEAN_TARGETS := $(foreach target,$(TARGETS),$(target:.bin=)-clean)

SUBFOLDERS := $(MODULES)

all: mesosphere.bin

clean: $(CLEAN_TARGETS)
	@rm -f mesosphere.bin

mesosphere.bin: $(TARGETS)
	@python build_mesosphere.py
	@echo "Built mesosphere.bin..."

$(TARGETS): check_libmeso
	$(MAKE) -C $(@:.bin=)
	@cp $(@:.bin=)/$(@) $(@)

check_libmeso:
	@$(MAKE) --no-print-directory -C ../libraries/libmesosphere

$(CLEAN_TARGETS):
	$(MAKE) -C $(@:-clean=) clean
	@rm -f $(@:-clean=).bin

.PHONY: all clean $(CLEAN_TARGETS)
