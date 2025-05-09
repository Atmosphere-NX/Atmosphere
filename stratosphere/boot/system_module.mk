#---------------------------------------------------------------------------------
# pull in common stratosphere sysmodule configuration
#---------------------------------------------------------------------------------
THIS_MAKEFILE := $(abspath $(lastword $(MAKEFILE_LIST)))
CURRENT_DIRECTORY := $(abspath $(dir $(THIS_MAKEFILE)))
include $(dir $(abspath $(lastword $(MAKEFILE_LIST))))/../../libraries/config/templates/stratosphere.mk

ATMOSPHERE_SYSTEM_MODULE_TARGETS := kip

#---------------------------------------------------------------------------------
# no real need to edit anything past this point unless you need to add additional
# rules for different file extensions
#---------------------------------------------------------------------------------
ifneq ($(__RECURSIVE__),1)
#---------------------------------------------------------------------------------

export TOPDIR	:=	$(CURDIR)

export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
			$(foreach dir,$(DATA),$(CURDIR)/$(dir)) $(ATMOSPHERE_LIBRARIES_DIR)/../fusee/$(ATMOSPHERE_BOOT_OUT_DIR)

CFILES      :=	$(call FIND_SOURCE_FILES,$(SOURCES),c)
CPPFILES    :=	$(call FIND_SOURCE_FILES,$(SOURCES),cpp)
SFILES      :=	$(call FIND_SOURCE_FILES,$(SOURCES),s)

BINFILES	:=	$(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*)))

#---------------------------------------------------------------------------------
# use CXX for linking C++ projects, CC for standard C
#---------------------------------------------------------------------------------
ifeq ($(strip $(CPPFILES)),)
#---------------------------------------------------------------------------------
	export LD	:=	$(CC)
#---------------------------------------------------------------------------------
else
#---------------------------------------------------------------------------------
	export LD	:=	$(CXX)
#---------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------

export OFILES	:=	$(addsuffix .o,$(BINFILES)) \
			$(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)

export INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
			$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
			$(foreach dir,$(AMS_LIBDIRS),-I$(dir)/include) \
			-I$(CURDIR)/$(ATMOSPHERE_BUILD_DIR)

export LIBPATHS	:=	$(foreach dir,$(LIBDIRS),-L$(dir)/lib) $(foreach dir,$(AMS_LIBDIRS),-L$(dir)/$(ATMOSPHERE_LIBRARY_DIR))

export BUILD_EXEFS_SRC := $(TOPDIR)/$(EXEFS_SRC)

ifeq ($(strip $(CONFIG_JSON)),)
	jsons := $(wildcard *.json)
	ifneq (,$(findstring $(TARGET).json,$(jsons)))
		export APP_JSON := $(TOPDIR)/$(TARGET).json
	else
		ifneq (,$(findstring config.json,$(jsons)))
			export APP_JSON := $(TOPDIR)/config.json
		endif
	endif
else
	export APP_JSON := $(TOPDIR)/$(CONFIG_JSON)
endif

.PHONY: clean all check_lib check_fusee

#---------------------------------------------------------------------------------
all: $(ATMOSPHERE_OUT_DIR) $(ATMOSPHERE_BUILD_DIR) $(ATMOSPHERE_LIBRARIES_DIR)/libstratosphere/$(ATMOSPHERE_LIBRARY_DIR)/libstratosphere.a $(ATMOSPHERE_LIBRARIES_DIR)/../fusee/$(ATMOSPHERE_BOOT_OUT_DIR)/fusee.bin
	@$(MAKE) __RECURSIVE__=1 OUTPUT=$(CURDIR)/$(ATMOSPHERE_OUT_DIR)/$(TARGET) \
	DEPSDIR=$(CURDIR)/$(ATMOSPHERE_BUILD_DIR) \
	--no-print-directory -C $(ATMOSPHERE_BUILD_DIR) \
	-f $(THIS_MAKEFILE)

$(ATMOSPHERE_LIBRARIES_DIR)/libstratosphere/$(ATMOSPHERE_LIBRARY_DIR)/libstratosphere.a: check_lib
	@$(SILENTCMD)echo "Checked library."

$(ATMOSPHERE_LIBRARIES_DIR)/../fusee/$(ATMOSPHERE_BOOT_OUT_DIR)/fusee.bin: check_fusee
	@$(SILENTCMD)echo "Checked fusee."

ifeq ($(ATMOSPHERE_CHECKED_LIBSTRATOSPHERE),1)
check_lib:
else
check_lib:
	@$(MAKE) --no-print-directory -C $(ATMOSPHERE_LIBRARIES_DIR)/libstratosphere -f $(ATMOSPHERE_LIBRARIES_DIR)/libstratosphere/libstratosphere.mk
endif

ifeq ($(ATMOSPHERE_CHECKED_FUSEE),1)
check_fusee:
else
check_fusee:
	@$(MAKE) --no-print-directory -C $(ATMOSPHERE_LIBRARIES_DIR)/../fusee -f $(ATMOSPHERE_LIBRARIES_DIR)/../fusee/Makefile $(ATMOSPHERE_MAKEFILE_TARGET)
	@$(SILENTCMD)echo $(VPATH)
endif

$(ATMOSPHERE_OUT_DIR) $(ATMOSPHERE_BUILD_DIR):
	@[ -d $@ ] || mkdir -p $@

#---------------------------------------------------------------------------------
clean:
	@echo clean ...
	@rm -fr $(ATMOSPHERE_OUT_DIR) $(ATMOSPHERE_BUILD_DIR)


#---------------------------------------------------------------------------------
else
.PHONY:	all

DEPENDS	:=	$(OFILES:.o=.d)

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
all	:	$(foreach target,$(ATMOSPHERE_SYSTEM_MODULE_TARGETS),$(OUTPUT).$(target))

$(OUTPUT).kip :	$(OUTPUT).elf
$(OUTPUT).nsp :	$(OUTPUT).nso $(OUTPUT).npdm
$(OUTPUT).nso :	$(OUTPUT).elf

$(OUTPUT).elf :	$(OFILES)

$(OFILES) : $(ATMOSPHERE_LIBRARIES_DIR)/libstratosphere/$(ATMOSPHERE_LIBRARY_DIR)/libstratosphere.a

%.npdm  :   %.npdm.json
	@echo built ... $< $@
	@npdmtool $< $@
	@echo built ... $(notdir $@)

boot_power_utils.o: CXXFLAGS += --embed-dir="$(ATMOSPHERE_LIBRARIES_DIR)/../fusee/$(ATMOSPHERE_BOOT_OUT_DIR)/"

-include $(DEPENDS)

#---------------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------------
