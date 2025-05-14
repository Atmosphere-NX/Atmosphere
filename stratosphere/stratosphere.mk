#---------------------------------------------------------------------------------
# pull in common atmosphere configuration
#---------------------------------------------------------------------------------
THIS_MAKEFILE     := $(abspath $(lastword $(MAKEFILE_LIST)))
CURRENT_DIRECTORY := $(abspath $(dir $(THIS_MAKEFILE)))
include $(CURRENT_DIRECTORY)/../libraries/config/common.mk

ALL_MODULES := loader boot ncm pm sm ams_mitm spl eclct.stub ro creport fatal dmnt boot2 erpt pgl jpegdec LogManager cs htc TioServer dmnt.gen2 memlet

all: $(ALL_MODULES)

$(ALL_MODULES): check_lib
	@$(SILENTCMD)echo Checking $@...
	@$(MAKE) --no-print-directory -C $(CURRENT_DIRECTORY)/$@ -f $(CURRENT_DIRECTORY)/$@/system_module.mk ATMOSPHERE_CHECKED_LIBSTRATOSPHERE=1

$(foreach module,$(ALL_MODULES),clean-$(module)):
	@$(SILENTCMD)echo Cleaning $(@:clean-%=%)...
	@$(MAKE) --no-print-directory -C $(CURRENT_DIRECTORY)/$(@:clean-%=%) -f $(CURRENT_DIRECTORY)/$(@:clean-%=%)/system_module.mk clean

ifeq ($(ATMOSPHERE_CHECKED_LIBSTRATOSPHERE),1)
check_lib:
else
check_lib:
	@$(MAKE) --no-print-directory -C $(ATMOSPHERE_LIBRARIES_DIR)/libstratosphere -f $(ATMOSPHERE_LIBRARIES_DIR)/libstratosphere/libstratosphere.mk
endif

clean: $(foreach module,$(ALL_MODULES),clean-$(module))

.PHONY: all clean check_lib $(foreach module,$(ALL_MODULES),$(module) clean-$(module))