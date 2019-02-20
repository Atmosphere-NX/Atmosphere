TOPTARGETS := all clean dist
AMSBRANCH := $(shell git symbolic-ref --short HEAD)
AMSREV := $(AMSBRANCH)-$(shell git rev-parse --short HEAD)

ifneq (, $(strip $(shell git status --porcelain 2>/dev/null)))
    AMSREV := $(AMSREV)-dirty
endif

COMPONENTS := fusee stratosphere exosphere thermosphere troposphere

all: $(COMPONENTS)

thermosphere:
	$(MAKE) -C thermosphere all

exosphere: thermosphere
	$(MAKE) -C exosphere all

stratosphere: exosphere
	$(MAKE) -C stratosphere all

troposphere: stratosphere
	$(MAKE) -C troposphere all

fusee: exosphere stratosphere
	$(MAKE) -C $@ all

clean:
	$(MAKE) -C fusee clean
	rm -rf out

dist: all
	$(eval MAJORVER = $(shell grep '\ATMOSPHERE_RELEASE_VERSION_MAJOR\b' common/include/atmosphere/version.h \
		| tr -s [:blank:] \
		| cut -d' ' -f3))
	$(eval MINORVER = $(shell grep '\ATMOSPHERE_RELEASE_VERSION_MINOR\b' common/include/atmosphere/version.h \
		| tr -s [:blank:] \
		| cut -d' ' -f3))
	$(eval MICROVER = $(shell grep '\ATMOSPHERE_RELEASE_VERSION_MICRO\b' common/include/atmosphere/version.h \
		| tr -s [:blank:] \
		| cut -d' ' -f3))
	$(eval AMSVER = $(MAJORVER).$(MINORVER).$(MICROVER)-$(AMSREV))
	rm -rf atmosphere-$(AMSVER)
	rm -rf out
	mkdir atmosphere-$(AMSVER)
	mkdir atmosphere-$(AMSVER)/atmosphere
	mkdir atmosphere-$(AMSVER)/switch
	mkdir -p atmosphere-$(AMSVER)/atmosphere/titles/0100000000000036
	mkdir -p atmosphere-$(AMSVER)/atmosphere/titles/0100000000000034
	mkdir -p atmosphere-$(AMSVER)/atmosphere/titles/0100000000000032
	cp fusee/fusee-primary/fusee-primary.bin atmosphere-$(AMSVER)/atmosphere/reboot_payload.bin
	cp fusee/fusee-secondary/fusee-secondary.bin atmosphere-$(AMSVER)/atmosphere/fusee-secondary.bin
	cp common/defaults/BCT.ini atmosphere-$(AMSVER)/atmosphere/BCT.ini
	cp common/defaults/loader.ini atmosphere-$(AMSVER)/atmosphere/loader.ini
	cp common/defaults/system_settings.ini atmosphere-$(AMSVER)/atmosphere/system_settings.ini
	cp -r common/defaults/kip_patches atmosphere-$(AMSVER)/atmosphere/kip_patches
	cp stratosphere/creport/creport.nsp atmosphere-$(AMSVER)/atmosphere/titles/0100000000000036/exefs.nsp
	cp stratosphere/fatal/fatal.nsp atmosphere-$(AMSVER)/atmosphere/titles/0100000000000034/exefs.nsp
	cp stratosphere/eclct.stub/eclct.stub.nsp atmosphere-$(AMSVER)/atmosphere/titles/0100000000000032/exefs.nsp
	cp troposphere/reboot_to_payload/reboot_to_payload.nro atmosphere-$(AMSVER)/switch/reboot_to_payload.nro
	mkdir -p atmosphere-$(AMSVER)/atmosphere/titles/0100000000000032/flags
	touch atmosphere-$(AMSVER)/atmosphere/titles/0100000000000032/flags/boot2.flag
	cd atmosphere-$(AMSVER); zip -r ../atmosphere-$(AMSVER).zip ./*; cd ../;
	rm -r atmosphere-$(AMSVER)
	mkdir out
	mv atmosphere-$(AMSVER).zip out/atmosphere-$(AMSVER).zip
	cp fusee/fusee-primary/fusee-primary.bin out/fusee-primary.bin
	cp sept/sept-primary/sept-primary.bin out/sept-primary.bin


.PHONY: $(TOPTARGETS) $(COMPONENTS)
