.PHONY: all arm64 arm clean arm64-clean arm-clean

all: arm64 arm

arm64:
	$(MAKE) -f arm64.mk

arm:
	$(MAKE) -f arm.mk

#---------------------------------------------------------------------------------

clean: arm64-clean arm-clean

arm64-clean:
	$(MAKE) -f arm64.mk clean

arm-clean:
	$(MAKE) -f arm.mk clean
