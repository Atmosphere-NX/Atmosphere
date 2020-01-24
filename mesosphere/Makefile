TARGETS := kernel kernel_ldr
CLEAN_TARGETS := $(foreach target,$(TARGETS),$(target)-clean)

SUBFOLDERS := $(MODULES)

all: mesosphere.bin

clean: $(CLEAN_TARGETS)
	@rm -f mesosphere.bin

mesosphere.bin: $(TARGETS)
	@python build_mesosphere.py
	@echo "Built mesosphere.bin..."

$(TARGETS):
	$(MAKE) -C $@

$(CLEAN_TARGETS):
	$(MAKE) -C $(@:-clean=) clean

.PHONY: all clean $(TARGETS) $(CLEAN_TARGETS)
