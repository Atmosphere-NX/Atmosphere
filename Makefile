TOPTARGETS := all clean

all: fusee
fusee:
	$(MAKE) -C $@ all

clean:
	$(MAKE) -C fusee clean

.PHONY: $(TOPTARGETS) fusee
