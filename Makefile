SUBFOLDERS := exosphere fusee/fusee-primary fusee/fusee-secondary stratosphere/libstratosphere thermosphere

TOPTARGETS := all clean

$(TOPTARGETS): $(SUBFOLDERS)

$(SUBFOLDERS):
	$(MAKE) -C $@ $(MAKECMDGOALS)

.PHONY: $(TOPTARGETS) $(SUBFOLDERS)
