ATMOSPHERE_BUILD_CONFIGS :=
KEFIR_ROOT_DIR ?= /mnt/d/git/dev/_kefir

ifndef KEF_VERSION
ifneq ("$(wildcard $(KEFIR_ROOT_DIR)/version)","")
KEF_VERSION := $(shell cat $(KEFIR_ROOT_DIR)/version)
else
KEF_VERSION := UNK
endif
endif

NPROCS := $(shell nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 1)

all: nx_release

clean: clean-nx_release

THIS_MAKEFILE     := $(abspath $(lastword $(MAKEFILE_LIST)))
CURRENT_DIRECTORY := $(abspath $(dir $(THIS_MAKEFILE)))
LIBNX_DIR := ../libnx

KEF_8GB_DIR := $(KEFIR_ROOT_DIR)/8gb
KEF_OC_DIR  := $(KEFIR_ROOT_DIR)/oc
KEF_40MB_DIR := $(KEFIR_ROOT_DIR)/40mb

define ATMOSPHERE_ADD_TARGET

ATMOSPHERE_BUILD_CONFIGS += $(strip $1)

$(strip $1):
	@echo "Building $(strip $1)"
	@$$(MAKE) -f $(CURRENT_DIRECTORY)/atmosphere.mk ATMOSPHERE_MAKEFILE_TARGET="$(strip $1)" ATMOSPHERE_BUILD_NAME="$(strip $2)" ATMOSPHERE_BOARD="$(strip $3)" ATMOSPHERE_CPU="$(strip $4)" $(strip $5)

clean-$(strip $1):
	@echo "Cleaning $(strip $1)"
	@$$(MAKE) -f $(CURRENT_DIRECTORY)/atmosphere.mk clean ATMOSPHERE_MAKEFILE_TARGET="$(strip $1)" ATMOSPHERE_BUILD_NAME="$(strip $2)" ATMOSPHERE_BOARD="$(strip $3)" ATMOSPHERE_CPU="$(strip $4)" $(strip $5)

endef

define ATMOSPHERE_ADD_TARGETS

$(eval $(call ATMOSPHERE_ADD_TARGET, $(strip $1)_release, release, $(strip $2), $(strip $3), \
    ATMOSPHERE_BUILD_SETTINGS="$(strip $4)" \
))

$(eval $(call ATMOSPHERE_ADD_TARGET, $(strip $1)_debug, debug, $(strip $2), $(strip $3), \
    ATMOSPHERE_BUILD_SETTINGS="$(strip $4) -DAMS_BUILD_FOR_DEBUGGING" ATMOSPHERE_BUILD_FOR_DEBUGGING=1 \
))

$(eval $(call ATMOSPHERE_ADD_TARGET, $(strip $1)_audit, audit, $(strip $2), $(strip $3), \
    ATMOSPHERE_BUILD_SETTINGS="$(strip $4) -DAMS_BUILD_FOR_AUDITING" ATMOSPHERE_BUILD_FOR_DEBUGGING=1 ATMOSPHERE_BUILD_FOR_AUDITING=1 \
))

endef

$(eval $(call ATMOSPHERE_ADD_TARGETS, nx, nx-hac-001, arm-cortex-a57,))

clean-all: $(foreach config,$(ATMOSPHERE_BUILD_CONFIGS),clean-$(config))

update:
	@echo "---------------------------------------------------------"
	@echo ">>> Updating devkitPro packages..."
	@echo "---------------------------------------------------------"
	sudo dkp-pacman -Syu
	@echo "---------------------------------------------------------"
	@echo ">>> Updating libnx repository..."
	@echo "---------------------------------------------------------"
	@if [ -d "$(LIBNX_DIR)" ]; then \
		(cd $(LIBNX_DIR) && git pull) || exit 1; \
	else \
		echo "Error: libnx directory '$(abspath $(LIBNX_DIR))' not found." >&2; \
		exit 1; \
	fi
	@echo "---------------------------------------------------------"
	@echo ">>> Building and installing libnx..."
	@echo "---------------------------------------------------------"
	@if [ -d "$(LIBNX_DIR)" ]; then \
		(cd $(LIBNX_DIR) && $(MAKE) install -j$(NPROCS)) || exit 1; \
	else \
		echo "Error: libnx directory '$(abspath $(LIBNX_DIR))' not found." >&2; \
		exit 1; \
	fi
	@echo "---------------------------------------------------------"
	@echo ">>> devkitPro and libnx update complete."
	@echo "---------------------------------------------------------"

clean-logo:
	$(info ---------------------------------------------------------)
	$(info  Building logo in $(CURDIR))
	$(info ---------------------------------------------------------)
	git checkout master
	python3 $(CURDIR)/stratosphere/boot/source/bmp_to_array.py
	$(MAKE) -C $(CURDIR)/stratosphere/boot clean -j$(NPROCS)
	@if ! git diff --quiet; then \
		git add . && git commit -m "changed logo"; \
	else \
		echo "Nothing to commit on master"; \
	fi

	git checkout 8gb_DRAM
	python3 $(CURDIR)/stratosphere/boot/source/bmp_to_array.py
	$(MAKE) -C $(CURDIR)/stratosphere/boot clean -j$(NPROCS)
	@if ! git diff --quiet; then \
		git add . && git commit -m "changed logo"; \
	else \
		echo "Nothing to commit on 8gb_DRAM"; \
	fi
	git checkout master

kefir-version:
	cd libraries/libstratosphere && $(MAKE) -j$(NPROCS) clean && cd ../../stratosphere/ams_mitm && $(MAKE) -j$(NPROCS) clean && cd ../.. && $(MAKE) -j$(NPROCS)

8gb_DRAM:
	$(info ---------------------------------------------------------)
	$(info             Built with 8GB DRAM!)
	$(info ---------------------------------------------------------)
	git checkout 8gb_DRAM
	git merge master --no-edit
	$(MAKE) clean -j$(NPROCS)
	$(MAKE) -f atmosphere.mk package3 ATMOSPHERE_GIT_REVISION="K$(KEF_VERSION)-8GB" -j$(NPROCS)
	$(MAKE) -C fusee -j$(NPROCS)
	mkdir -p $(KEF_8GB_DIR)/atmosphere/
	mkdir -p $(KEF_8GB_DIR)/bootloader/payloads/
	cp fusee/out/nintendo_nx_arm_armv4t/release/package3 $(KEF_8GB_DIR)/atmosphere/package3
	cp fusee/out/nintendo_nx_arm_armv4t/release/fusee.bin $(KEF_8GB_DIR)/bootloader/payloads/fusee.bin
	python utilities/insert_splash_screen.py ~/dev/_kefir/bootlogo/splash_logo.png $(KEF_8GB_DIR)/atmosphere/package3
	$(info ---------------------------------------------------------)
	$(info             FINISH building with 8GB DRAM!)
	$(info ---------------------------------------------------------)
	git checkout master

oc:
	$(info ---------------------------------------------------------)
	$(info                     Built with OC)
	$(info ---------------------------------------------------------)
	git checkout oc
	git merge master --no-edit
	$(MAKE) clean -j$(NPROCS)
	$(MAKE) -C stratosphere/loader ATMOSPHERE_GIT_REVISION="K$(KEF_VERSION)-OC" -j$(NPROCS)
	mkdir -p $(KEF_OC_DIR)/atmosphere/kips/
	mkdir -p $(KEFIR_ROOT_DIR)/kefir/config/oc/atmosphere/kips/
	cp stratosphere/loader/out/nintendo_nx_arm64_armv8a/release/loader.kip $(KEF_OC_DIR)/atmosphere/kips/kefir.kip
	cp stratosphere/loader/out/nintendo_nx_arm64_armv8a/release/loader.kip $(KEFIR_ROOT_DIR)/kefir/config/oc/atmosphere/kips/kefir.kip
	$(info ---------------------------------------------------------)
	$(info                   FINISH building OC!)
	$(info ---------------------------------------------------------)
	git checkout master

40mb:
	$(info ---------------------------------------------------------)
	$(info                   Building 40MB Mesosphere!)
	$(info ---------------------------------------------------------)
	git checkout 40mb
	git merge master --no-edit
	$(MAKE) clean -j$(NPROCS)
	$(MAKE) -f atmosphere.mk package3 ATMOSPHERE_GIT_REVISION="K$(KEF_VERSION)-40MB" -j$(NPROCS)
	mkdir -p $(KEF_40MB_DIR)/atmosphere/
	cp fusee/out/nintendo_nx_arm_armv4t/release/package3 $(KEF_40MB_DIR)/atmosphere/package3
	$(info ---------------------------------------------------------)
	$(info             FINISH building 40MB!)
	$(info ---------------------------------------------------------)
	git checkout master

kefir:
	git checkout master
	$(MAKE) clean -j$(NPROCS)
	$(MAKE) clean-logo 
	$(MAKE) 8gb_DRAM
	$(MAKE) oc
	$(MAKE) 40mb
	$(MAKE) nx_release -j$(NPROCS)

.PHONY: all clean clean-all kefir-version update clean-logo clear 8gb_DRAM oc kefir 40mb $(foreach config,$(ATMOSPHERE_BUILD_CONFIGS), $(config) clean-$(config))