MODULES := loader pm sm boot ams_mitm spl eclct.stub ro creport fatal dmnt

SUBFOLDERS := libstratosphere $(MODULES)

TOPTARGETS := all clean

$(TOPTARGETS): $(SUBFOLDERS)

$(SUBFOLDERS):
	$(MAKE) -C $@ $(MAKECMDGOALS)

$(MODULES): libstratosphere

.PHONY: $(TOPTARGETS) $(SUBFOLDERS)
