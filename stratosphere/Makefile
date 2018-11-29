KIPS := loader pm sm boot fs_mitm set_mitm creport fatal

#TODO: boot2 ?

SUBFOLDERS := libstratosphere $(KIPS)

TOPTARGETS := all clean

$(TOPTARGETS): $(SUBFOLDERS)

$(SUBFOLDERS):
	$(MAKE) -C $@ $(MAKECMDGOALS)

$(KIPS): libstratosphere

.PHONY: $(TOPTARGETS) $(SUBFOLDERS)
