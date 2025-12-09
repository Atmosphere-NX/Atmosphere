ATMOSPHERE_BUILD_CONFIGS :=

all: nx_release

clean: clean-nx_release

THIS_MAKEFILE     := $(abspath $(lastword $(MAKEFILE_LIST)))
CURRENT_DIRECTORY := $(abspath $(dir $(THIS_MAKEFILE)))

# --- Configuration for update target ---
# Шлях до каталогу libnx відносно цього Makefile
LIBNX_DIR := ../libnx
# ---------------------------------------

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

# --- Custom Targets ---

update:
	@echo "---------------------------------------------------------"
	@echo ">>> Updating devkitPro packages..."
	@echo "---------------------------------------------------------"
	sudo dkp-pacman -Syu
	@echo "---------------------------------------------------------"
	@echo ">>> Updating libnx repository in $(abspath $(LIBNX_DIR))..."
	@echo "---------------------------------------------------------"
	@if [ -d "$(LIBNX_DIR)" ]; then \
		(cd $(LIBNX_DIR) && git pull) || exit 1; \
	else \
		echo "Error: libnx directory '$(abspath $(LIBNX_DIR))' not found." >&2; \
		exit 1; \
	fi
	@echo "---------------------------------------------------------"
	@echo ">>> Building and installing libnx from $(abspath $(LIBNX_DIR))..."
	@echo "---------------------------------------------------------"
	@if [ -d "$(LIBNX_DIR)" ]; then \
		(cd $(LIBNX_DIR) && $(MAKE) install -j12) || exit 1; \
	else \
		echo "Error: libnx directory '$(abspath $(LIBNX_DIR))' not found (should not happen if previous step passed)." >&2; \
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
	$(MAKE) -C $(CURDIR)/stratosphere/boot clean -j12
	@if ! git diff --quiet; then \
		git add . && git commit -m "changed logo"; \
	else \
		echo "Nothing to commit on master"; \
	fi

	git checkout 8gb_DRAM
	python3 $(CURDIR)/stratosphere/boot/source/bmp_to_array.py
	$(MAKE) -C $(CURDIR)/stratosphere/boot clean -j12
	@if ! git diff --quiet; then \
		git add . && git commit -m "changed logo"; \
	else \
		echo "Nothing to commit on 8gb_DRAM"; \
	fi

	git checkout master


kefir-version:
	cd libraries/libstratosphere && $(MAKE) -j12 clean && cd ../../stratosphere/ams_mitm && $(MAKE) -j12 clean && cd ../.. && $(MAKE) -j12

clear:
	$(MAKE) clean -j12
	$(MAKE) -j12

8gb_DRAM:
	$(info ---------------------------------------------------------)
	$(info                   Built with 8GB DRAM!)
	$(info ---------------------------------------------------------)
	git checkout 8gb_DRAM
	git merge master --no-edit
	$(MAKE) -f atmosphere.mk package3 -j12
	$(MAKE) -C fusee -j12
	mkdir -p /mnt/d/git/dev/_kefir/8gb/atmosphere/
	mkdir -p /mnt/d/git/dev/_kefir/8gb/bootloader/payloads/
	cp fusee/out/nintendo_nx_arm_armv4t/release/package3 /mnt/d/git/dev/_kefir/8gb/atmosphere/package3
	cp fusee/out/nintendo_nx_arm_armv4t/release/fusee.bin /mnt/d/git/dev/_kefir/8gb/bootloader/payloads/fusee.bin
	python utilities/insert_splash_screen.py ~/dev/_kefir/bootlogo/splash_logo.png /mnt/d/git/dev/_kefir/8gb/atmosphere/package3
	$(info ---------------------------------------------------------)
	$(info             FINISH building with 8GB DRAM!)
	$(info ---------------------------------------------------------)
	git checkout master

8gb_DRAM-clean:
	$(info ---------------------------------------------------------)
	$(info                   Built with 8GB DRAM!)
	$(info ---------------------------------------------------------)
	git checkout 8gb_DRAM
	git merge master --no-edit
	$(MAKE) clean -j12
	$(MAKE) -f atmosphere.mk package3 -j12
	$(MAKE) -C fusee -j12
	mkdir -p /mnt/d/git/dev/_kefir/8gb/atmosphere/
	mkdir -p /mnt/d/git/dev/_kefir/8gb/bootloader/payloads/
	cp fusee/out/nintendo_nx_arm_armv4t/release/package3 /mnt/d/git/dev/_kefir/8gb/atmosphere/package3
	cp fusee/out/nintendo_nx_arm_armv4t/release/fusee.bin /mnt/d/git/dev/_kefir/8gb/bootloader/payloads/fusee.bin
	python utilities/insert_splash_screen.py ~/dev/_kefir/bootlogo/splash_logo.png /mnt/d/git/dev/_kefir/8gb/atmosphere/package3
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
	$(MAKE) -C stratosphere/loader -j12
	cp stratosphere/loader/out/nintendo_nx_arm64_armv8a/release/loader.kip /mnt/d/git/dev/_kefir/oc/atmosphere/kips/kefir.kip
	cp stratosphere/loader/out/nintendo_nx_arm64_armv8a/release/loader.kip /mnt/d/git/dev/_kefir/kefir/config/oc/atmosphere/kips/kefir.kip
	$(info ---------------------------------------------------------)
	$(info                   FINISH building OC!)
	$(info ---------------------------------------------------------)
	git checkout master

oc-clean:
	$(info ---------------------------------------------------------)
	$(info                     Built with OC)
	$(info ---------------------------------------------------------)
	git checkout oc
	git merge master --no-edit
	$(MAKE) clean -j12
	$(MAKE) -C stratosphere/loader -j12
	cp stratosphere/loader/out/nintendo_nx_arm64_armv8a/release/loader.kip /mnt/d/git/dev/_kefir/oc/atmosphere/kips/kefir.kip
	cp stratosphere/loader/out/nintendo_nx_arm64_armv8a/release/loader.kip /mnt/d/git/dev/_kefir/kefir/config/oc/atmosphere/kips/kefir.kip
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
	
	$(MAKE) -f atmosphere.mk mesosphere -j12
	
	$(MAKE) -f atmosphere.mk package3 -j12
	
	mkdir -p ~/dev/_kefir/40mb/atmosphere/
	cp fusee/out/nintendo_nx_arm_armv4t/release/package3 ~/dev/_kefir/40mb/atmosphere/package3
	
	$(info ---------------------------------------------------------)
	$(info             FINISH building 40MB!)
	$(info ---------------------------------------------------------)
	git checkout master

40mb-clean:
	$(info ---------------------------------------------------------)
	$(info             Building 40MB Mesosphere (CLEAN)!)
	$(info ---------------------------------------------------------)
	git checkout 40mb
	git merge master --no-edit
	
	$(MAKE) clean -j12
	
	$(MAKE) -f atmosphere.mk mesosphere -j12
	
	$(MAKE) -f atmosphere.mk package3 -j12
	
	mkdir -p ~/dev/_kefir/40mb/atmosphere/
	cp fusee/out/nintendo_nx_arm_armv4t/release/package3 ~/dev/_kefir/40mb/atmosphere/package3
	
	$(info ---------------------------------------------------------)
	$(info          FINISH building 40MB (Clean)!)
	$(info ---------------------------------------------------------)
	git checkout master

kefir: 
	# update
	git checkout master
	$(MAKE) clean -j12
	$(MAKE) clean-logo 
	$(MAKE) 8gb_DRAM-clean
	$(MAKE) oc-clean
	$(MAKE) 40mb-clean
	$(MAKE) nx_release -j12

kefir-fast: update
	git checkout master
	# $(MAKE) clean-logo 
	$(MAKE) 8gb_DRAM
	$(MAKE) oc
	$(MAKE) 40mb
	$(MAKE) nx_release -j12

.PHONY: all clean clean-all kefir-version update clean-logo clear hekate 8gb_DRAM 8gb_DRAM-clean oc oc-clean kefir kefir-fast 40mb 40mb-clean $(foreach config,$(ATMOSPHERE_BUILD_CONFIGS), $(config) clean-$(config))