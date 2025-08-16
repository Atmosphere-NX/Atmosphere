#---------------------------------------------------------------------------------
# pull in common atmosphere configuration
#---------------------------------------------------------------------------------
THIS_MAKEFILE     := $(abspath $(lastword $(MAKEFILE_LIST)))
CURRENT_DIRECTORY := $(abspath $(dir $(THIS_MAKEFILE)))
include $(CURRENT_DIRECTORY)/../libraries/config/common.mk

all: $(CURRENT_DIRECTORY)/$(ATMOSPHERE_OUT_DIR)/mesosphere.bin

$(CURRENT_DIRECTORY)/$(ATMOSPHERE_OUT_DIR)/mesosphere.bin: $(CURRENT_DIRECTORY)/kernel/$(ATMOSPHERE_OUT_DIR)/kernel.bin $(CURRENT_DIRECTORY)/kernel_ldr/$(ATMOSPHERE_OUT_DIR)/kernel_ldr.bin $(CURRENT_DIRECTORY)/$(ATMOSPHERE_OUT_DIR)
	$(SILENTCMD)$(PYTHON) build_mesosphere.py $(CURRENT_DIRECTORY)/kernel_ldr/$(ATMOSPHERE_OUT_DIR)/kernel_ldr.bin $(CURRENT_DIRECTORY)/kernel/$(ATMOSPHERE_OUT_DIR)/kernel.bin $(CURRENT_DIRECTORY)/$(ATMOSPHERE_OUT_DIR)/mesosphere.bin
	@echo "Built mesosphere.bin..."

$(CURRENT_DIRECTORY)/kernel/$(ATMOSPHERE_OUT_DIR)/kernel.bin: check_kernel
	@$(SILENTCMD)echo "Checked kernel."

$(CURRENT_DIRECTORY)/kernel_ldr/$(ATMOSPHERE_OUT_DIR)/kernel_ldr.bin: check_kernel_ldr
	@$(SILENTCMD)echo "Checked kernel ldr."

$(ATMOSPHERE_LIBRARIES_DIR)/libmesosphere/$(ATMOSPHERE_LIBRARY_DIR)/libmesosphere.a: check_lib
	@$(SILENTCMD)echo "Checked library."

check_kernel: $(ATMOSPHERE_LIBRARIES_DIR)/libmesosphere/$(ATMOSPHERE_LIBRARY_DIR)/libmesosphere.a
	@$(SILENTCMD)echo "Checking kernel..."
	@$(MAKE) --no-print-directory -C $(CURRENT_DIRECTORY)/kernel -f $(CURRENT_DIRECTORY)/kernel/kernel.mk ATMOSPHERE_CHECKED_LIBMESOSPHERE=1

check_kernel_ldr: $(ATMOSPHERE_LIBRARIES_DIR)/libmesosphere/$(ATMOSPHERE_LIBRARY_DIR)/libmesosphere.a
	@$(SILENTCMD)echo "Checking kernel ldr..."
	@$(MAKE) --no-print-directory -C $(CURRENT_DIRECTORY)/kernel_ldr -f $(CURRENT_DIRECTORY)/kernel_ldr/kernel_ldr.mk ATMOSPHERE_CHECKED_LIBMESOSPHERE=1

ifeq ($(ATMOSPHERE_CHECKED_LIBMESOSPHERE),1)
check_lib:
else
check_lib:
	@$(MAKE) --no-print-directory -C $(ATMOSPHERE_LIBRARIES_DIR)/libmesosphere -f $(ATMOSPHERE_LIBRARIES_DIR)/libmesosphere/libmesosphere.mk
endif


$(CURRENT_DIRECTORY)/$(ATMOSPHERE_OUT_DIR):
	@[ -d $@ ] || mkdir -p $@

clean:
	@echo clean ...
	@$(MAKE) --no-print-directory -C $(CURRENT_DIRECTORY)/kernel -f $(CURRENT_DIRECTORY)/kernel/kernel.mk clean
	@$(MAKE) --no-print-directory -C $(CURRENT_DIRECTORY)/kernel_ldr -f $(CURRENT_DIRECTORY)/kernel_ldr/kernel_ldr.mk clean
	@rm -fr $(CURRENT_DIRECTORY)/$(ATMOSPHERE_OUT_DIR)/mesosphere.bin
	@for i in $(CURRENT_DIRECTORY)/$(ATMOSPHERE_OUT_DIR); do [ -d $$i ] && rmdir $$i 2>/dev/null || true; done

.PHONY: all clean check_lib check_kernel check_kernel_ldr