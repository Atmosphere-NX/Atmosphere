MODULES := loader pm sm boot ams_mitm eclct.stub creport fatal

SUBFOLDERS := libstratosphere $(MODULES)

TOPTARGETS := all clean

$(TOPTARGETS): $(SUBFOLDERS)

$(SUBFOLDERS):
	$(MAKE) -C $@ $(MAKECMDGOALS)

$(MODULES): libstratosphere

.PHONY: $(TOPTARGETS) $(SUBFOLDERS)
