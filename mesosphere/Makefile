TARGETS := kernel kernel_ldr
CLEAN_TARGETS := $(foreach target,$(TARGETS),$(target)-clean)

SUBFOLDERS := $(MODULES)

all: mesosphere.bin

clean: $(CLEAN_TARGETS)
	@rm -f mesosphere.bin

mesosphere.bin: $(TARGETS)
	@echo "todo: py script"

$(TARGETS):
	$(MAKE) -C $@

$(CLEAN_TARGETS):
	$(MAKE) -C $(@:-clean=) clean

.PHONY: all clean $(TARGETS) $(CLEAN_TARGETS)
