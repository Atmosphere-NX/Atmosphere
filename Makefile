TOPTARGETS := all clean dist
AMSREV := $(shell git rev-parse --short HEAD)
ifneq (, $(strip $(shell git status --porcelain 2>/dev/null)))
    AMSREV := $(AMSREV)-dirty
endif

all: fusee creport
fusee:
	$(MAKE) -C $@ all

creport:
	$(MAKE) -C stratosphere/creport all

clean:
	$(MAKE) -C fusee clean
	rm -rf out
    
dist: fusee creport
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
	mkdir -p atmosphere-$(AMSVER)/atmosphere/titles/0100000000000036
	cp fusee/fusee-secondary/fusee-secondary.bin atmosphere-$(AMSVER)/fusee-secondary.bin
	cp common/defaults/BCT.ini atmosphere-$(AMSVER)/BCT.ini
	cp common/defaults/loader.ini atmosphere-$(AMSVER)/atmosphere/loader.ini
	cp stratosphere/creport/creport.nsp atmosphere-$(AMSVER)/atmosphere/titles/0100000000000036/exefs.nsp
	cd atmosphere-$(AMSVER); zip -r ../atmosphere-$(AMSVER).zip ./*; cd ../;
	rm -r atmosphere-$(AMSVER)
	mkdir out
	mv atmosphere-$(AMSVER).zip out/atmosphere-$(AMSVER).zip
	cp fusee/fusee-primary/fusee-primary.bin out/fusee-primary.bin
   

.PHONY: $(TOPTARGETS) fusee
