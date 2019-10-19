MODULES := loader pm sm boot ams_mitm spl eclct.stub ro creport fatal dmnt boot2

SUBFOLDERS := libstratosphere $(MODULES)

TOPTARGETS := all clean

$(TOPTARGETS): $(SUBFOLDERS)

$(SUBFOLDERS):
	$(MAKE) -C $@ $(MAKECMDGOALS)

$(MODULES): libstratosphere

.PHONY: $(TOPTARGETS) $(SUBFOLDERS)
