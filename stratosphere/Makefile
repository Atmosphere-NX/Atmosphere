KIPS := loader pm sm boot

#TODO: boot2 ?

SUBFOLDERS := libstratosphere $(KIPS)

TOPTARGETS := all clean

$(TOPTARGETS): $(SUBFOLDERS)

$(SUBFOLDERS):
	$(MAKE) -C $@ $(MAKECMDGOALS)

$(KIPS): libstratosphere

.PHONY: $(TOPTARGETS) $(SUBFOLDERS)
