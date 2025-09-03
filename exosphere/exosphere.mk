#---------------------------------------------------------------------------------
# pull in common atmosphere configuration
#---------------------------------------------------------------------------------
THIS_MAKEFILE     := $(abspath $(lastword $(MAKEFILE_LIST)))
CURRENT_DIRECTORY := $(abspath $(dir $(THIS_MAKEFILE)))
include $(CURRENT_DIRECTORY)/../libraries/config/common.mk

all: $(CURRENT_DIRECTORY)/$(ATMOSPHERE_OUT_DIR)/exosphere.bin $(CURRENT_DIRECTORY)/warmboot/$(ATMOSPHERE_BOOT_OUT_DIR)/warmboot.bin $(CURRENT_DIRECTORY)/mariko_fatal/$(ATMOSPHERE_OUT_DIR)/mariko_fatal.bin

$(CURRENT_DIRECTORY)/$(ATMOSPHERE_OUT_DIR)/exosphere.bin: $(CURRENT_DIRECTORY)/loader_stub/$(ATMOSPHERE_OUT_DIR)/loader_stub.bin $(CURRENT_DIRECTORY)/$(ATMOSPHERE_OUT_DIR)
	@cp $(CURRENT_DIRECTORY)/loader_stub/$(ATMOSPHERE_OUT_DIR)/loader_stub.bin $(CURRENT_DIRECTORY)/$(ATMOSPHERE_OUT_DIR)/exosphere.bin
	@printf LENY >> $(CURRENT_DIRECTORY)/$(ATMOSPHERE_OUT_DIR)/exosphere.bin
	@echo "Built exosphere.bin..."

$(CURRENT_DIRECTORY)/loader_stub/$(ATMOSPHERE_OUT_DIR)/loader_stub.bin: check_loader_stub
	@$(SILENTCMD)echo "Checked loader stub."

$(CURRENT_DIRECTORY)/program/$(ATMOSPHERE_OUT_DIR)/program.lz4: check_program
	@$(SILENTCMD)echo "Checked program."

$(CURRENT_DIRECTORY)/warmboot/$(ATMOSPHERE_BOOT_OUT_DIR)/warmboot.bin: check_warmboot
	@$(SILENTCMD)echo "Checked warmboot."

$(CURRENT_DIRECTORY)/mariko_fatal/$(ATMOSPHERE_OUT_DIR)/mariko_fatal.bin: check_mariko_fatal
	@$(SILENTCMD)echo "Checked mariko fatal."

$(ATMOSPHERE_LIBRARIES_DIR)/libexosphere/$(ATMOSPHERE_LIBRARY_DIR)/libexosphere.a: check_lib
	@$(SILENTCMD)echo "Checked library."

ifneq ($(strip $(ATMOSPHERE_LIBRARY_DIR)),$(strip $(ATMOSPHERE_BOOT_LIBRARY_DIR)))
$(ATMOSPHERE_LIBRARIES_DIR)/libexosphere/$(ATMOSPHERE_BOOT_LIBRARY_DIR)/libexosphere.a: check_boot_lib
	@$(SILENTCMD)echo "Checked boot library."
endif

check_loader_stub: $(CURRENT_DIRECTORY)/program/$(ATMOSPHERE_OUT_DIR)/program.lz4
	@$(MAKE) --no-print-directory -C $(CURRENT_DIRECTORY)/loader_stub -f $(CURRENT_DIRECTORY)/loader_stub/loader_stub.mk ATMOSPHERE_CHECKED_LIBEXOSPHERE=1 ATMOSPHERE_CHECKED_BOOT_LIBEXOSPHERE=1 ATMOSPHERE_CHECKED_EXOSPHERE_PROGRAM=1

check_program: $(ATMOSPHERE_LIBRARIES_DIR)/libexosphere/$(ATMOSPHERE_LIBRARY_DIR)/libexosphere.a $(ATMOSPHERE_LIBRARIES_DIR)/libexosphere/$(ATMOSPHERE_BOOT_LIBRARY_DIR)/libexosphere.a
	@$(MAKE) --no-print-directory -C $(CURRENT_DIRECTORY)/program -f $(CURRENT_DIRECTORY)/program/program.mk ATMOSPHERE_CHECKED_LIBEXOSPHERE=1 ATMOSPHERE_CHECKED_BOOT_LIBEXOSPHERE=1

check_mariko_fatal: $(ATMOSPHERE_LIBRARIES_DIR)/libexosphere/$(ATMOSPHERE_LIBRARY_DIR)/libexosphere.a
	@$(MAKE) --no-print-directory -C $(CURRENT_DIRECTORY)/mariko_fatal -f $(CURRENT_DIRECTORY)/mariko_fatal/mariko_fatal.mk ATMOSPHERE_CHECKED_LIBEXOSPHERE=1 ATMOSPHERE_CHECKED_BOOT_LIBEXOSPHERE=1

check_warmboot: $(ATMOSPHERE_LIBRARIES_DIR)/libexosphere/$(ATMOSPHERE_BOOT_LIBRARY_DIR)/libexosphere.a
	@$(MAKE) --no-print-directory -C $(CURRENT_DIRECTORY)/warmboot -f $(CURRENT_DIRECTORY)/warmboot/warmboot.mk ATMOSPHERE_CHECKED_LIBEXOSPHERE=1 ATMOSPHERE_CHECKED_BOOT_LIBEXOSPHERE=1 ATMOSPHERE_CPU="$(strip $(ATMOSPHERE_BOOT_CPU))"

ifeq ($(ATMOSPHERE_CHECKED_LIBEXOSPHERE),1)
check_lib:
else
check_lib:
	@$(MAKE) --no-print-directory -C $(ATMOSPHERE_LIBRARIES_DIR)/libexosphere -f $(ATMOSPHERE_LIBRARIES_DIR)/libexosphere/libexosphere.mk
endif

ifeq ($(ATMOSPHERE_CHECKED_BOOT_LIBEXOSPHERE),1)
check_boot_lib:
else
check_boot_lib:
	@$(MAKE) --no-print-directory -C $(ATMOSPHERE_LIBRARIES_DIR)/libexosphere -f $(ATMOSPHERE_LIBRARIES_DIR)/libexosphere/libexosphere.mk ATMOSPHERE_CPU="$(strip $(ATMOSPHERE_BOOT_CPU))"
endif

$(CURRENT_DIRECTORY)/$(ATMOSPHERE_OUT_DIR):
	@[ -d $@ ] || mkdir -p $@

clean:
	@echo clean ...
	@$(MAKE) --no-print-directory -C $(CURRENT_DIRECTORY)/loader_stub -f $(CURRENT_DIRECTORY)/loader_stub/loader_stub.mk clean
	@$(MAKE) --no-print-directory -C $(CURRENT_DIRECTORY)/program -f $(CURRENT_DIRECTORY)/program/program.mk clean
	@$(MAKE) --no-print-directory -C $(CURRENT_DIRECTORY)/mariko_fatal -f $(CURRENT_DIRECTORY)/mariko_fatal/mariko_fatal.mk clean
	@$(MAKE) --no-print-directory -C $(CURRENT_DIRECTORY)/warmboot -f $(CURRENT_DIRECTORY)/warmboot/warmboot.mk clean ATMOSPHERE_CPU="$(strip $(ATMOSPHERE_BOOT_CPU))"
	@$(MAKE) --no-print-directory -C $(ATMOSPHERE_LIBRARIES_DIR)/libexosphere -f $(ATMOSPHERE_LIBRARIES_DIR)/libexosphere/libexosphere.mk clean
	@$(MAKE) --no-print-directory -C $(ATMOSPHERE_LIBRARIES_DIR)/libexosphere -f $(ATMOSPHERE_LIBRARIES_DIR)/libexosphere/libexosphere.mk clean ATMOSPHERE_CPU="$(strip $(ATMOSPHERE_BOOT_CPU))"
	@rm -fr $(CURRENT_DIRECTORY)/$(ATMOSPHERE_OUT_DIR)
	@for i in $(CURRENT_DIRECTORY)/$(ATMOSPHERE_OUT_DIR); do [ -d $$i ] && rmdir $$i 2>/dev/null || true; done

.PHONY: all clean check_lib check_boot_lib check_loader_stub check_program check_mariko_fatal check_warmboot