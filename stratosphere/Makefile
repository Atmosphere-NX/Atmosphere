MODULES := loader ncm pm sm boot ams_mitm spl eclct.stub ro creport fatal dmnt boot2 erpt

SUBFOLDERS := $(MODULES)

TOPTARGETS := all clean

$(TOPTARGETS): $(SUBFOLDERS)

$(SUBFOLDERS):
	$(MAKE) -C $@ $(MAKECMDGOALS)

.PHONY: $(TOPTARGETS) $(SUBFOLDERS)
