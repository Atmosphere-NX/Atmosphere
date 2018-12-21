MODULES := loader pm sm boot fs_mitm set_mitm creport fatal

SUBFOLDERS := libstratosphere $(MODULES)

TOPTARGETS := all clean

$(TOPTARGETS): $(SUBFOLDERS)

$(SUBFOLDERS):
	$(MAKE) -C $@ $(MAKECMDGOALS)

$(MODULES): libstratosphere

.PHONY: $(TOPTARGETS) $(SUBFOLDERS)
