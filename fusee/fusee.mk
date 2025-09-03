#---------------------------------------------------------------------------------
# pull in common atmosphere configuration
#---------------------------------------------------------------------------------
THIS_MAKEFILE     := $(abspath $(lastword $(MAKEFILE_LIST)))
CURRENT_DIRECTORY := $(abspath $(dir $(THIS_MAKEFILE)))
include $(CURRENT_DIRECTORY)/../libraries/config/common.mk

all: $(CURRENT_DIRECTORY)/$(ATMOSPHERE_OUT_DIR)/fusee.bin

$(CURRENT_DIRECTORY)/$(ATMOSPHERE_OUT_DIR)/fusee.bin: $(CURRENT_DIRECTORY)/loader_stub/$(ATMOSPHERE_OUT_DIR)/loader_stub.bin $(CURRENT_DIRECTORY)/$(ATMOSPHERE_OUT_DIR)
	@cp $(CURRENT_DIRECTORY)/loader_stub/$(ATMOSPHERE_OUT_DIR)/loader_stub.bin $(CURRENT_DIRECTORY)/$(ATMOSPHERE_OUT_DIR)/fusee.bin
	@echo "Built fusee.bin..."

$(CURRENT_DIRECTORY)/loader_stub/$(ATMOSPHERE_OUT_DIR)/loader_stub.bin: check_loader_stub
	@$(SILENTCMD)echo "Checked loader stub."

$(CURRENT_DIRECTORY)/program/$(ATMOSPHERE_OUT_DIR)/program.bin: check_program
	@$(SILENTCMD)echo "Checked program."

$(ATMOSPHERE_LIBRARIES_DIR)/libexosphere/$(ATMOSPHERE_LIBRARY_DIR)/libexosphere.a: check_lib
	@$(SILENTCMD)echo "Checked library."

check_loader_stub: $(ATMOSPHERE_LIBRARIES_DIR)/libexosphere/$(ATMOSPHERE_LIBRARY_DIR)/libexosphere.a $(CURRENT_DIRECTORY)/program/$(ATMOSPHERE_OUT_DIR)/program.bin
	@$(SILENTCMD)echo "Checking loader stub..."
	@$(MAKE) --no-print-directory -C $(CURRENT_DIRECTORY)/loader_stub -f $(CURRENT_DIRECTORY)/loader_stub/loader_stub.mk ATMOSPHERE_CHECKED_LIBEXOSPHERE=1 ATMOSPHERE_CHECKED_FUSEE_PROGRAM=1

check_program: $(ATMOSPHERE_LIBRARIES_DIR)/libexosphere/$(ATMOSPHERE_LIBRARY_DIR)/libexosphere.a
	@$(SILENTCMD)echo "Checking program..."
	@$(MAKE) --no-print-directory -C $(CURRENT_DIRECTORY)/program -f $(CURRENT_DIRECTORY)/program/program.mk ATMOSPHERE_CHECKED_LIBEXOSPHERE=1

ifeq ($(ATMOSPHERE_CHECKED_LIBEXOSPHERE),1)
check_lib:
else
check_lib:
	@$(MAKE) --no-print-directory -C $(ATMOSPHERE_LIBRARIES_DIR)/libexosphere -f $(ATMOSPHERE_LIBRARIES_DIR)/libexosphere/libexosphere.mk
endif

$(CURRENT_DIRECTORY)/$(ATMOSPHERE_OUT_DIR):
	@[ -d $@ ] || mkdir -p $@

clean:
	@echo clean ...
	@$(MAKE) --no-print-directory -C $(CURRENT_DIRECTORY)/loader_stub -f $(CURRENT_DIRECTORY)/loader_stub/loader_stub.mk clean
	@$(MAKE) --no-print-directory -C $(CURRENT_DIRECTORY)/program -f $(CURRENT_DIRECTORY)/program/program.mk clean
	@rm -fr $(CURRENT_DIRECTORY)/$(ATMOSPHERE_OUT_DIR)
	@for i in $(CURRENT_DIRECTORY)/$(ATMOSPHERE_OUT_DIR); do [ -d $$i ] && rmdir $$i 2>/dev/null || true; done

.PHONY: all clean check_lib check_loader_stub check_program