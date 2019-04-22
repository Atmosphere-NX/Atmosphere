TOPTARGETS := all clean dist
AMSBRANCH := $(shell git symbolic-ref --short HEAD)
AMSHASH := $(shell git rev-parse --short HEAD)
AMSREV := $(AMSBRANCH)-$(AMSHASH)

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

sept: exosphere
	$(MAKE) -C sept all

fusee: exosphere stratosphere sept
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
	mkdir atmosphere-$(AMSVER)/sept
	mkdir atmosphere-$(AMSVER)/switch
	mkdir -p atmosphere-$(AMSVER)/atmosphere/titles/010000000000000D
	mkdir -p atmosphere-$(AMSVER)/atmosphere/titles/0100000000000032
	mkdir -p atmosphere-$(AMSVER)/atmosphere/titles/0100000000000034
	mkdir -p atmosphere-$(AMSVER)/atmosphere/titles/0100000000000036
	mkdir -p atmosphere-$(AMSVER)/atmosphere/titles/0100000000000037
	mkdir -p atmosphere-$(AMSVER)/atmosphere/fatal_errors
	cp fusee/fusee-primary/fusee-primary.bin atmosphere-$(AMSVER)/atmosphere/reboot_payload.bin
	cp fusee/fusee-secondary/fusee-secondary.bin atmosphere-$(AMSVER)/atmosphere/fusee-secondary.bin
	cp fusee/fusee-secondary/fusee-secondary.bin atmosphere-$(AMSVER)/sept/payload.bin
	cp sept/sept-primary/sept-primary.bin atmosphere-$(AMSVER)/sept/sept-primary.bin
	cp sept/sept-secondary/sept-secondary.bin atmosphere-$(AMSVER)/sept/sept-secondary.bin
	cp sept/sept-secondary/sept-secondary.enc atmosphere-$(AMSVER)/sept/sept-secondary.enc
	cp common/defaults/BCT.ini atmosphere-$(AMSVER)/atmosphere/BCT.ini
	cp common/defaults/loader.ini atmosphere-$(AMSVER)/atmosphere/loader.ini
	cp common/defaults/system_settings.ini atmosphere-$(AMSVER)/atmosphere/system_settings.ini
	cp -r common/defaults/kip_patches atmosphere-$(AMSVER)/atmosphere/kip_patches
	cp -r common/defaults/hbl_html atmosphere-$(AMSVER)/atmosphere/hbl_html
	cp stratosphere/dmnt/dmnt.nsp atmosphere-$(AMSVER)/atmosphere/titles/010000000000000D/exefs.nsp
	cp stratosphere/eclct.stub/eclct.stub.nsp atmosphere-$(AMSVER)/atmosphere/titles/0100000000000032/exefs.nsp
	cp stratosphere/fatal/fatal.nsp atmosphere-$(AMSVER)/atmosphere/titles/0100000000000034/exefs.nsp
	cp stratosphere/creport/creport.nsp atmosphere-$(AMSVER)/atmosphere/titles/0100000000000036/exefs.nsp
	cp stratosphere/ro/ro.nsp atmosphere-$(AMSVER)/atmosphere/titles/0100000000000037/exefs.nsp
	mkdir -p atmosphere-$(AMSVER)/atmosphere/titles/0100000000000032/flags
	touch atmosphere-$(AMSVER)/atmosphere/titles/0100000000000032/flags/boot2.flag
	mkdir -p atmosphere-$(AMSVER)/atmosphere/titles/0100000000000037/flags
	touch atmosphere-$(AMSVER)/atmosphere/titles/0100000000000037/flags/boot2.flag
	cp troposphere/reboot_to_payload/reboot_to_payload.nro atmosphere-$(AMSVER)/switch/reboot_to_payload.nro
	cd atmosphere-$(AMSVER); zip -r ../atmosphere-$(AMSVER).zip ./*; cd ../;
	rm -r atmosphere-$(AMSVER)
	mkdir out
	mv atmosphere-$(AMSVER).zip out/atmosphere-$(AMSVER).zip
	cp fusee/fusee-primary/fusee-primary.bin out/fusee-primary.bin


.PHONY: $(TOPTARGETS) $(COMPONENTS)
