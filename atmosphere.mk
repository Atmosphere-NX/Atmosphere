#---------------------------------------------------------------------------------
# pull in common atmosphere configuration
#---------------------------------------------------------------------------------
THIS_MAKEFILE     := $(abspath $(lastword $(MAKEFILE_LIST)))
CURRENT_DIRECTORY := $(abspath $(dir $(THIS_MAKEFILE)))
include $(CURRENT_DIRECTORY)/libraries/config/common.mk

# Get Atmosphere version fields
ATMOSPHERE_MAJOR_VERSION               := $(shell grep 'define ATMOSPHERE_RELEASE_VERSION_MAJOR\b' $(ATMOSPHERE_LIBRARIES_DIR)/libvapours/include/vapours/ams/ams_api_version.h | tr -s [:blank:] | cut -d' ' -f3)
ATMOSPHERE_MINOR_VERSION               := $(shell grep 'define ATMOSPHERE_RELEASE_VERSION_MINOR\b' $(ATMOSPHERE_LIBRARIES_DIR)/libvapours/include/vapours/ams/ams_api_version.h | tr -s [:blank:] | cut -d' ' -f3)
ATMOSPHERE_MICRO_VERSION               := $(shell grep 'define ATMOSPHERE_RELEASE_VERSION_MICRO\b' $(ATMOSPHERE_LIBRARIES_DIR)/libvapours/include/vapours/ams/ams_api_version.h | tr -s [:blank:] | cut -d' ' -f3)
ATMOSPHERE_SUPPORTED_HOS_MAJOR_VERSION := $(shell grep 'define ATMOSPHERE_SUPPORTED_HOS_VERSION_MAJOR\b' $(ATMOSPHERE_LIBRARIES_DIR)/libvapours/include/vapours/ams/ams_api_version.h | tr -s [:blank:] | cut -d' ' -f3)
ATMOSPHERE_SUPPORTED_HOS_MINOR_VERSION := $(shell grep 'define ATMOSPHERE_SUPPORTED_HOS_VERSION_MINOR\b' $(ATMOSPHERE_LIBRARIES_DIR)/libvapours/include/vapours/ams/ams_api_version.h | tr -s [:blank:] | cut -d' ' -f3)
ATMOSPHERE_SUPPORTED_HOS_MICRO_VERSION := $(shell grep 'define ATMOSPHERE_SUPPORTED_HOS_VERSION_MICRO\b' $(ATMOSPHERE_LIBRARIES_DIR)/libvapours/include/vapours/ams/ams_api_version.h | tr -s [:blank:] | cut -d' ' -f3)

ATMOSPHERE_VERSION := $(ATMOSPHERE_MAJOR_VERSION).$(ATMOSPHERE_MINOR_VERSION).$(ATMOSPHERE_MICRO_VERSION)-$(ATMOSPHERE_GIT_REVISION)

dist: dist-no-debug
	$(eval DIST_DIR = $(CURRENT_DIRECTORY)/$(ATMOSPHERE_OUT_DIR)/atmosphere-$(ATMOSPHERE_VERSION)-debug)
	rm -rf $(DIST_DIR)
	mkdir $(DIST_DIR)
	cp $(CURRENT_DIRECTORY)/fusee/loader_stub/$(ATMOSPHERE_BOOT_OUT_DIR)/loader_stub.elf $(DIST_DIR)/fusee-loader-stub.elf
	cp $(CURRENT_DIRECTORY)/fusee/program/$(ATMOSPHERE_BOOT_OUT_DIR)/program.elf $(DIST_DIR)/fusee-program.elf
	cp $(CURRENT_DIRECTORY)/exosphere/loader_stub/$(ATMOSPHERE_OUT_DIR)/loader_stub.elf $(DIST_DIR)/exosphere-loader-stub.elf
	cp $(CURRENT_DIRECTORY)/exosphere/program/$(ATMOSPHERE_OUT_DIR)/program.elf $(DIST_DIR)/exosphere-program.elf
	cp $(CURRENT_DIRECTORY)/exosphere/warmboot/$(ATMOSPHERE_BOOT_OUT_DIR)/warmboot.elf $(DIST_DIR)/exosphere-warmboot.elf
	cp $(CURRENT_DIRECTORY)/exosphere/mariko_fatal/$(ATMOSPHERE_OUT_DIR)/mariko_fatal.elf $(DIST_DIR)/exosphere-mariko-fatal.elf
	cp $(CURRENT_DIRECTORY)/exosphere/program/sc7fw/$(ATMOSPHERE_BOOT_OUT_DIR)/sc7fw.elf $(DIST_DIR)/exosphere-sc7fw.elf
	cp $(CURRENT_DIRECTORY)/exosphere/program/rebootstub/$(ATMOSPHERE_BOOT_OUT_DIR)/rebootstub.elf $(DIST_DIR)/exosphere-rebootstub.elf
	cp $(CURRENT_DIRECTORY)/mesosphere/kernel_ldr/$(ATMOSPHERE_OUT_DIR)/kernel_ldr.elf $(DIST_DIR)/kernel_ldr.elf
	cp $(CURRENT_DIRECTORY)/mesosphere/kernel/$(ATMOSPHERE_OUT_DIR)/kernel.elf $(DIST_DIR)/kernel.elf
	cp $(CURRENT_DIRECTORY)/stratosphere/ams_mitm/$(ATMOSPHERE_OUT_DIR)/ams_mitm.elf $(DIST_DIR)/ams_mitm.elf
	cp $(CURRENT_DIRECTORY)/stratosphere/boot/$(ATMOSPHERE_OUT_DIR)/boot.elf $(DIST_DIR)/boot.elf
	cp $(CURRENT_DIRECTORY)/stratosphere/boot2/$(ATMOSPHERE_OUT_DIR)/boot2.elf $(DIST_DIR)/boot2.elf
	cp $(CURRENT_DIRECTORY)/stratosphere/creport/$(ATMOSPHERE_OUT_DIR)/creport.elf $(DIST_DIR)/creport.elf
	cp $(CURRENT_DIRECTORY)/stratosphere/cs/$(ATMOSPHERE_OUT_DIR)/cs.elf $(DIST_DIR)/cs.elf
	cp $(CURRENT_DIRECTORY)/stratosphere/dmnt/$(ATMOSPHERE_OUT_DIR)/dmnt.elf $(DIST_DIR)/dmnt.elf
	cp $(CURRENT_DIRECTORY)/stratosphere/dmnt.gen2/$(ATMOSPHERE_OUT_DIR)/dmnt.gen2.elf $(DIST_DIR)/dmnt.gen2.elf
	cp $(CURRENT_DIRECTORY)/stratosphere/eclct.stub/$(ATMOSPHERE_OUT_DIR)/eclct.stub.elf $(DIST_DIR)/eclct.stub.elf
	cp $(CURRENT_DIRECTORY)/stratosphere/erpt/$(ATMOSPHERE_OUT_DIR)/erpt.elf $(DIST_DIR)/erpt.elf
	cp $(CURRENT_DIRECTORY)/stratosphere/fatal/$(ATMOSPHERE_OUT_DIR)/fatal.elf $(DIST_DIR)/fatal.elf
	cp $(CURRENT_DIRECTORY)/stratosphere/htc/$(ATMOSPHERE_OUT_DIR)/htc.elf $(DIST_DIR)/htc.elf
	cp $(CURRENT_DIRECTORY)/stratosphere/jpegdec/$(ATMOSPHERE_OUT_DIR)/jpegdec.elf $(DIST_DIR)/jpegdec.elf
	cp $(CURRENT_DIRECTORY)/stratosphere/loader/$(ATMOSPHERE_OUT_DIR)/loader.elf $(DIST_DIR)/loader.elf
	cp $(CURRENT_DIRECTORY)/stratosphere/LogManager/$(ATMOSPHERE_OUT_DIR)/LogManager.elf $(DIST_DIR)/LogManager.elf
	cp $(CURRENT_DIRECTORY)/stratosphere/ncm/$(ATMOSPHERE_OUT_DIR)/ncm.elf $(DIST_DIR)/ncm.elf
	cp $(CURRENT_DIRECTORY)/stratosphere/pgl/$(ATMOSPHERE_OUT_DIR)/pgl.elf $(DIST_DIR)/pgl.elf
	cp $(CURRENT_DIRECTORY)/stratosphere/pm/$(ATMOSPHERE_OUT_DIR)/pm.elf $(DIST_DIR)/pm.elf
	cp $(CURRENT_DIRECTORY)/stratosphere/ro/$(ATMOSPHERE_OUT_DIR)/ro.elf $(DIST_DIR)/ro.elf
	cp $(CURRENT_DIRECTORY)/stratosphere/sm/$(ATMOSPHERE_OUT_DIR)/sm.elf $(DIST_DIR)/sm.elf
	cp $(CURRENT_DIRECTORY)/stratosphere/spl/$(ATMOSPHERE_OUT_DIR)/spl.elf $(DIST_DIR)/spl.elf
	cp $(CURRENT_DIRECTORY)/stratosphere/TioServer/$(ATMOSPHERE_OUT_DIR)/TioServer.elf $(DIST_DIR)/TioServer.elf
	cp $(CURRENT_DIRECTORY)/stratosphere/memlet/$(ATMOSPHERE_OUT_DIR)/memlet.elf $(DIST_DIR)/memlet.elf
	cp $(CURRENT_DIRECTORY)/troposphere/daybreak/daybreak.elf $(DIST_DIR)/daybreak.elf
	cp $(CURRENT_DIRECTORY)/troposphere/haze/haze.elf $(DIST_DIR)/haze.elf
	cp $(CURRENT_DIRECTORY)/troposphere/reboot_to_payload/reboot_to_payload.elf $(DIST_DIR)/reboot_to_payload.elf
	cd $(DIST_DIR); zip -r ../atmosphere-$(ATMOSPHERE_VERSION)-debug.zip ./*; cd ../;
	rm -rf $(DIST_DIR)

dist-no-debug: package3 $(CURRENT_DIRECTORY)/$(ATMOSPHERE_OUT_DIR)
	$(eval DIST_DIR = $(CURRENT_DIRECTORY)/$(ATMOSPHERE_OUT_DIR)/atmosphere-$(ATMOSPHERE_VERSION))
	rm -rf $(CURRENT_DIRECTORY)/$(ATMOSPHERE_OUT_DIR)/*
	rm -rf $(DIST_DIR)
	mkdir $(DIST_DIR)
	mkdir $(DIST_DIR)/atmosphere
	mkdir $(DIST_DIR)/switch
	mkdir -p $(DIST_DIR)/atmosphere/fatal_errors
	mkdir -p $(DIST_DIR)/atmosphere/config_templates
	mkdir -p $(DIST_DIR)/atmosphere/config
	mkdir -p $(DIST_DIR)/atmosphere/flags
	cp fusee/$(ATMOSPHERE_BOOT_OUT_DIR)/fusee.bin $(DIST_DIR)/atmosphere/reboot_payload.bin
	cp fusee/$(ATMOSPHERE_BOOT_OUT_DIR)/package3 $(DIST_DIR)/atmosphere/package3
	cp config_templates/stratosphere.ini $(DIST_DIR)/atmosphere/config_templates/stratosphere.ini
	cp config_templates/override_config.ini $(DIST_DIR)/atmosphere/config_templates/override_config.ini
	cp config_templates/system_settings.ini $(DIST_DIR)/atmosphere/config_templates/system_settings.ini
	cp config_templates/exosphere.ini $(DIST_DIR)/atmosphere/config_templates/exosphere.ini
	mkdir -p config_templates/kip_patches
	cp -r config_templates/kip_patches $(DIST_DIR)/atmosphere/kip_patches
	cp -r config_templates/hbl_html $(DIST_DIR)/atmosphere/hbl_html
	mkdir -p $(DIST_DIR)/stratosphere_romfs/atmosphere/contents/0100000000000008
	mkdir -p $(DIST_DIR)/stratosphere_romfs/atmosphere/contents/010000000000000d
	mkdir -p $(DIST_DIR)/stratosphere_romfs/atmosphere/contents/0100000000000017
	mkdir -p $(DIST_DIR)/stratosphere_romfs/atmosphere/contents/010000000000002b
	mkdir -p $(DIST_DIR)/stratosphere_romfs/atmosphere/contents/0100000000000032
	mkdir -p $(DIST_DIR)/stratosphere_romfs/atmosphere/contents/0100000000000034
	mkdir -p $(DIST_DIR)/stratosphere_romfs/atmosphere/contents/0100000000000036
	mkdir -p $(DIST_DIR)/stratosphere_romfs/atmosphere/contents/0100000000000037
	#mkdir -p $(DIST_DIR)/stratosphere_romfs/atmosphere/contents/010000000000003c
	mkdir -p $(DIST_DIR)/stratosphere_romfs/atmosphere/contents/0100000000000042
	mkdir -p $(DIST_DIR)/stratosphere_romfs/atmosphere/contents/0100000000000420
	mkdir -p $(DIST_DIR)/stratosphere_romfs/atmosphere/contents/0100000000000421
	mkdir -p $(DIST_DIR)/stratosphere_romfs/atmosphere/contents/010000000000b240
	mkdir -p $(DIST_DIR)/stratosphere_romfs/atmosphere/contents/010000000000d609
	mkdir -p $(DIST_DIR)/stratosphere_romfs/atmosphere/contents/010000000000d623
	cp stratosphere/boot2/$(ATMOSPHERE_OUT_DIR)/boot2.nsp $(DIST_DIR)/stratosphere_romfs/atmosphere/contents/0100000000000008/exefs.nsp
	cp stratosphere/dmnt/$(ATMOSPHERE_OUT_DIR)/dmnt.nsp $(DIST_DIR)/stratosphere_romfs/atmosphere/contents/010000000000000d/exefs.nsp
	cp stratosphere/cs/$(ATMOSPHERE_OUT_DIR)/cs.nsp $(DIST_DIR)/stratosphere_romfs/atmosphere/contents/0100000000000017/exefs.nsp
	cp stratosphere/erpt/$(ATMOSPHERE_OUT_DIR)/erpt.nsp $(DIST_DIR)/stratosphere_romfs/atmosphere/contents/010000000000002b/exefs.nsp
	cp stratosphere/eclct.stub/$(ATMOSPHERE_OUT_DIR)/eclct.stub.nsp $(DIST_DIR)/stratosphere_romfs/atmosphere/contents/0100000000000032/exefs.nsp
	cp stratosphere/fatal/$(ATMOSPHERE_OUT_DIR)/fatal.nsp $(DIST_DIR)/stratosphere_romfs/atmosphere/contents/0100000000000034/exefs.nsp
	cp stratosphere/creport/$(ATMOSPHERE_OUT_DIR)/creport.nsp $(DIST_DIR)/stratosphere_romfs/atmosphere/contents/0100000000000036/exefs.nsp
	cp stratosphere/ro/$(ATMOSPHERE_OUT_DIR)/ro.nsp $(DIST_DIR)/stratosphere_romfs/atmosphere/contents/0100000000000037/exefs.nsp
	#cp stratosphere/jpegdec/$(ATMOSPHERE_OUT_DIR)/jpegdec.nsp $(DIST_DIR)/stratosphere_romfs/atmosphere/contents/010000000000003c/exefs.nsp
	cp stratosphere/pgl/$(ATMOSPHERE_OUT_DIR)/pgl.nsp $(DIST_DIR)/stratosphere_romfs/atmosphere/contents/0100000000000042/exefs.nsp
	cp stratosphere/LogManager/$(ATMOSPHERE_OUT_DIR)/LogManager.nsp $(DIST_DIR)/stratosphere_romfs/atmosphere/contents/0100000000000420/exefs.nsp
	cp stratosphere/htc/$(ATMOSPHERE_OUT_DIR)/htc.nsp $(DIST_DIR)/stratosphere_romfs/atmosphere/contents/010000000000b240/exefs.nsp
	cp stratosphere/dmnt.gen2/$(ATMOSPHERE_OUT_DIR)/dmnt.gen2.nsp $(DIST_DIR)/stratosphere_romfs/atmosphere/contents/010000000000d609/exefs.nsp
	cp stratosphere/TioServer/$(ATMOSPHERE_OUT_DIR)/TioServer.nsp $(DIST_DIR)/stratosphere_romfs/atmosphere/contents/010000000000d623/exefs.nsp
	cp stratosphere/memlet/$(ATMOSPHERE_OUT_DIR)/memlet.nsp $(DIST_DIR)/stratosphere_romfs/atmosphere/contents/0100000000000421/exefs.nsp
	@PATH="$(DEVKITPRO)/tools/bin:$$PATH" build_romfs $(DIST_DIR)/stratosphere_romfs $(DIST_DIR)/atmosphere/stratosphere.romfs
	rm -r $(DIST_DIR)/stratosphere_romfs
	cp troposphere/reboot_to_payload/reboot_to_payload.nro $(DIST_DIR)/switch/reboot_to_payload.nro
	cp troposphere/daybreak/daybreak.nro $(DIST_DIR)/switch/daybreak.nro
	cp troposphere/haze/haze.nro $(DIST_DIR)/switch/haze.nro
	cd $(DIST_DIR); zip -r ../atmosphere-$(ATMOSPHERE_VERSION).zip ./*; cd ../;
	rm -rf $(DIST_DIR)
	cp fusee/$(ATMOSPHERE_BOOT_OUT_DIR)/fusee.bin $(CURRENT_DIRECTORY)/$(ATMOSPHERE_OUT_DIR)/fusee.bin

package3: emummc fusee stratosphere mesosphere exosphere troposphere
	$(SILENTCMD)$(PYTHON) fusee/build_package3.py $(CURRENT_DIRECTORY) $(ATMOSPHERE_OUT_DIR) $(ATMOSPHERE_BOOT_OUT_DIR) $(ATMOSPHERE_GIT_HASH) $(ATMOSPHERE_MAJOR_VERSION) $(ATMOSPHERE_MINOR_VERSION) $(ATMOSPHERE_MICRO_VERSION) 0 $(ATMOSPHERE_SUPPORTED_HOS_MAJOR_VERSION) $(ATMOSPHERE_SUPPORTED_HOS_MINOR_VERSION) $(ATMOSPHERE_SUPPORTED_HOS_MICRO_VERSION) 0
	@echo "Built package3!"

emummc:
	$(MAKE) -C $(CURRENT_DIRECTORY)/emummc all

fusee: libexosphere_boot
	@$(MAKE) --no-print-directory -C $(CURRENT_DIRECTORY)/fusee -f $(CURRENT_DIRECTORY)/fusee/fusee.mk ATMOSPHERE_CPU="$(strip $(ATMOSPHERE_BOOT_CPU))"

exosphere: libexosphere libexosphere_boot
	@$(MAKE) --no-print-directory -C $(CURRENT_DIRECTORY)/exosphere -f $(CURRENT_DIRECTORY)/exosphere/exosphere.mk ATMOSPHERE_CHECKED_LIBEXOSPHERE=1 ATMOSPHERE_CHECKED_BOOT_LIBEXOSPHERE=1

stratosphere: fusee libstratosphere
	@$(MAKE) --no-print-directory -C $(CURRENT_DIRECTORY)/stratosphere -f $(CURRENT_DIRECTORY)/stratosphere/stratosphere.mk ATMOSPHERE_CHECKED_LIBSTRATOSPHERE=1 ATMOSPHERE_CHECKED_FUSEE=1

mesosphere: libmesosphere
	@$(MAKE) --no-print-directory -C $(CURRENT_DIRECTORY)/mesosphere -f $(CURRENT_DIRECTORY)/mesosphere/mesosphere.mk ATMOSPHERE_CHECKED_LIBMESOSPHERE=1

troposphere:
	$(MAKE) -C $(CURRENT_DIRECTORY)/troposphere all

libexosphere:
	@$(MAKE) --no-print-directory -C $(ATMOSPHERE_LIBRARIES_DIR)/libexosphere -f $(ATMOSPHERE_LIBRARIES_DIR)/libexosphere/libexosphere.mk

ifneq ($(strip $(ATMOSPHERE_LIBRARY_DIR)),$(strip $(ATMOSPHERE_BOOT_LIBRARY_DIR)))
libexosphere_boot:
	@$(MAKE) --no-print-directory -C $(ATMOSPHERE_LIBRARIES_DIR)/libexosphere -f $(ATMOSPHERE_LIBRARIES_DIR)/libexosphere/libexosphere.mk ATMOSPHERE_CPU="$(strip $(ATMOSPHERE_BOOT_CPU))"
else
libexosphere_boot: libexosphere
endif

libmesosphere:
	@$(MAKE) --no-print-directory -C $(ATMOSPHERE_LIBRARIES_DIR)/libmesosphere -f $(ATMOSPHERE_LIBRARIES_DIR)/libmesosphere/libmesosphere.mk

libstratosphere:
	@$(MAKE) --no-print-directory -C $(ATMOSPHERE_LIBRARIES_DIR)/libstratosphere -f $(ATMOSPHERE_LIBRARIES_DIR)/libstratosphere/libstratosphere.mk

clean:
	$(MAKE) -C $(CURRENT_DIRECTORY)/fusee -f $(CURRENT_DIRECTORY)/fusee/fusee.mk clean ATMOSPHERE_CPU="$(strip $(ATMOSPHERE_BOOT_CPU))"
	$(MAKE) -C $(CURRENT_DIRECTORY)/emummc clean
	$(MAKE) -C $(CURRENT_DIRECTORY)/exosphere -f $(CURRENT_DIRECTORY)/exosphere/exosphere.mk clean
	$(MAKE) -C $(CURRENT_DIRECTORY)/mesosphere -f $(CURRENT_DIRECTORY)/mesosphere/mesosphere.mk clean
	$(MAKE) -C $(CURRENT_DIRECTORY)/stratosphere -f $(CURRENT_DIRECTORY)/stratosphere/stratosphere.mk clean
	$(MAKE) -C $(CURRENT_DIRECTORY)/troposphere clean
	$(MAKE) -C $(ATMOSPHERE_LIBRARIES_DIR)/libstratosphere -f $(ATMOSPHERE_LIBRARIES_DIR)/libstratosphere/libstratosphere.mk clean
	$(MAKE) -C $(ATMOSPHERE_LIBRARIES_DIR)/libmesosphere -f $(ATMOSPHERE_LIBRARIES_DIR)/libmesosphere/libmesosphere.mk clean
	$(MAKE) -C $(ATMOSPHERE_LIBRARIES_DIR)/libexosphere -f $(ATMOSPHERE_LIBRARIES_DIR)/libexosphere/libexosphere.mk clean
	$(MAKE) -C $(ATMOSPHERE_LIBRARIES_DIR)/libexosphere -f $(ATMOSPHERE_LIBRARIES_DIR)/libexosphere/libexosphere.mk clean ATMOSPHERE_CPU="$(strip $(ATMOSPHERE_BOOT_CPU))"
	rm -rf $(CURRENT_DIRECTORY)/$(ATMOSPHERE_OUT_DIR)

$(CURRENT_DIRECTORY)/$(ATMOSPHERE_OUT_DIR) $(CURRENT_DIRECTORY)/$(ATMOSPHERE_BUILD_DIR):
	@[ -d $@ ] || mkdir -p $@

.PHONY: dist dist-no-debug clean package3 emummc fusee stratosphere mesosphere exosphere troposphere
