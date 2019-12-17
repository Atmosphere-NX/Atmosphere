TOPTARGETS := all clean dist
AMSBRANCH := $(shell git symbolic-ref --short HEAD)
AMSHASH := $(shell git rev-parse --short HEAD)
AMSREV := $(AMSBRANCH)-$(AMSHASH)

ifneq (, $(strip $(shell git status --porcelain 2>/dev/null)))
    AMSREV := $(AMSREV)-dirty
endif

COMPONENTS := fusee stratosphere mesosphere exosphere thermosphere troposphere libraries

all: $(COMPONENTS)

thermosphere:
	$(MAKE) -C thermosphere all

exosphere: thermosphere
	$(MAKE) -C exosphere all

stratosphere: exosphere libraries
	$(MAKE) -C stratosphere all

mesosphere: exosphere libraries
	$(MAKE) -C mesosphere all

troposphere: stratosphere
	$(MAKE) -C troposphere all

sept: exosphere
	$(MAKE) -C sept all

fusee: exosphere mesosphere stratosphere sept
	$(MAKE) -C $@ all

libraries:
	$(MAKE) -C libraries all

clean:
	$(MAKE) -C fusee clean
	rm -rf out

dist: all
	$(eval MAJORVER = $(shell grep 'define ATMOSPHERE_RELEASE_VERSION_MAJOR\b' libraries/libvapours/include/vapours/ams/ams_api_version.h \
		| tr -s [:blank:] \
		| cut -d' ' -f3))
	$(eval MINORVER = $(shell grep 'define ATMOSPHERE_RELEASE_VERSION_MINOR\b' libraries/libvapours/include/vapours/ams/ams_api_version.h \
		| tr -s [:blank:] \
		| cut -d' ' -f3))
	$(eval MICROVER = $(shell grep 'define ATMOSPHERE_RELEASE_VERSION_MICRO\b' libraries/libvapours/include/vapours/ams/ams_api_version.h \
		| tr -s [:blank:] \
		| cut -d' ' -f3))
	$(eval AMSVER = $(MAJORVER).$(MINORVER).$(MICROVER)-$(AMSREV))
	rm -rf atmosphere-$(AMSVER)
	rm -rf out
	mkdir atmosphere-$(AMSVER)
	mkdir atmosphere-$(AMSVER)/atmosphere
	mkdir atmosphere-$(AMSVER)/sept
	mkdir atmosphere-$(AMSVER)/switch
	mkdir -p atmosphere-$(AMSVER)/atmosphere/contents/0100000000000008
	mkdir -p atmosphere-$(AMSVER)/atmosphere/contents/010000000000000D
	mkdir -p atmosphere-$(AMSVER)/atmosphere/contents/0100000000000032
	mkdir -p atmosphere-$(AMSVER)/atmosphere/contents/0100000000000034
	mkdir -p atmosphere-$(AMSVER)/atmosphere/contents/0100000000000036
	mkdir -p atmosphere-$(AMSVER)/atmosphere/contents/0100000000000037
	mkdir -p atmosphere-$(AMSVER)/atmosphere/fatal_errors
	mkdir -p atmosphere-$(AMSVER)/atmosphere/config_templates
	mkdir -p atmosphere-$(AMSVER)/atmosphere/config
	cp fusee/fusee-primary/fusee-primary.bin atmosphere-$(AMSVER)/atmosphere/reboot_payload.bin
	cp fusee/fusee-mtc/fusee-mtc.bin atmosphere-$(AMSVER)/atmosphere/fusee-mtc.bin
	cp fusee/fusee-secondary/fusee-secondary.bin atmosphere-$(AMSVER)/atmosphere/fusee-secondary.bin
	cp fusee/fusee-secondary/fusee-secondary.bin atmosphere-$(AMSVER)/sept/payload.bin
	cp sept/sept-primary/sept-primary.bin atmosphere-$(AMSVER)/sept/sept-primary.bin
	cp sept/sept-secondary/sept-secondary.bin atmosphere-$(AMSVER)/sept/sept-secondary.bin
	cp sept/sept-secondary/sept-secondary_00.enc atmosphere-$(AMSVER)/sept/sept-secondary_00.enc
	cp sept/sept-secondary/sept-secondary_01.enc atmosphere-$(AMSVER)/sept/sept-secondary_01.enc
	cp config_templates/BCT.ini atmosphere-$(AMSVER)/atmosphere/config/BCT.ini
	cp config_templates/override_config.ini atmosphere-$(AMSVER)/atmosphere/config_templates/override_config.ini
	cp config_templates/system_settings.ini atmosphere-$(AMSVER)/atmosphere/config_templates/system_settings.ini
	cp -r config_templates/kip_patches atmosphere-$(AMSVER)/atmosphere/kip_patches
	cp -r config_templates/hbl_html atmosphere-$(AMSVER)/atmosphere/hbl_html
	cp stratosphere/boot2/boot2.nsp atmosphere-$(AMSVER)/atmosphere/contents/0100000000000008/exefs.nsp
	cp stratosphere/dmnt/dmnt.nsp atmosphere-$(AMSVER)/atmosphere/contents/010000000000000D/exefs.nsp
	cp stratosphere/eclct.stub/eclct.stub.nsp atmosphere-$(AMSVER)/atmosphere/contents/0100000000000032/exefs.nsp
	cp stratosphere/fatal/fatal.nsp atmosphere-$(AMSVER)/atmosphere/contents/0100000000000034/exefs.nsp
	cp stratosphere/creport/creport.nsp atmosphere-$(AMSVER)/atmosphere/contents/0100000000000036/exefs.nsp
	cp stratosphere/ro/ro.nsp atmosphere-$(AMSVER)/atmosphere/contents/0100000000000037/exefs.nsp
	mkdir -p atmosphere-$(AMSVER)/atmosphere/contents/0100000000000032/flags
	touch atmosphere-$(AMSVER)/atmosphere/contents/0100000000000032/flags/boot2.flag
	mkdir -p atmosphere-$(AMSVER)/atmosphere/contents/0100000000000037/flags
	touch atmosphere-$(AMSVER)/atmosphere/contents/0100000000000037/flags/boot2.flag
	cp troposphere/reboot_to_payload/reboot_to_payload.nro atmosphere-$(AMSVER)/switch/reboot_to_payload.nro
	cd atmosphere-$(AMSVER); zip -r ../atmosphere-$(AMSVER).zip ./*; cd ../;
	rm -r atmosphere-$(AMSVER)
	mkdir out
	mv atmosphere-$(AMSVER).zip out/atmosphere-$(AMSVER).zip
	cp fusee/fusee-primary/fusee-primary.bin out/fusee-primary.bin


.PHONY: $(TOPTARGETS) $(COMPONENTS)
