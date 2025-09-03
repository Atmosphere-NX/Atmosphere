#---------------------------------------------------------------------------------
# pull in common stratosphere sysmodule configuration
#---------------------------------------------------------------------------------
THIS_MAKEFILE := $(abspath $(lastword $(MAKEFILE_LIST)))
include $(dir $(abspath $(lastword $(MAKEFILE_LIST))))/../../libraries/config/templates/stratosphere.mk

ifeq ($(ATMOSPHERE_BOARD),nx-hac-001)
export BOARD_TARGET_SUFFIX := .kip
else ifeq ($(ATMOSPHERE_BOARD),generic_windows)
export BOARD_TARGET_SUFFIX := .exe
else ifeq ($(ATMOSPHERE_BOARD),generic_linux)
export BOARD_TARGET_SUFFIX :=
else ifeq ($(ATMOSPHERE_BOARD),generic_macos)
export BOARD_TARGET_SUFFIX :=
else
export BOARD_TARGET_SUFFIX := $(TARGET)
endif

#---------------------------------------------------------------------------------
# no real need to edit anything past this point unless you need to add additional
# rules for different file extensions
#---------------------------------------------------------------------------------
ifneq ($(__RECURSIVE__),1)
#---------------------------------------------------------------------------------

export TOPDIR	:=	$(CURDIR)

export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
			$(foreach dir,$(DATA),$(CURDIR)/$(dir))

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
			-I$(CURDIR)/$(BUILD)

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

.PHONY: clean all check_lib

#---------------------------------------------------------------------------------
all: $(ATMOSPHERE_OUT_DIR) $(ATMOSPHERE_BUILD_DIR) $(ATMOSPHERE_LIBRARIES_DIR)/libstratosphere/$(ATMOSPHERE_LIBRARY_DIR)/libstratosphere.a
	@$(MAKE) __RECURSIVE__=1 OUTPUT=$(CURDIR)/$(ATMOSPHERE_OUT_DIR)/$(TARGET) \
	DEPSDIR=$(CURDIR)/$(ATMOSPHERE_BUILD_DIR) \
	--no-print-directory -C $(ATMOSPHERE_BUILD_DIR) \
	-f $(THIS_MAKEFILE)

$(ATMOSPHERE_LIBRARIES_DIR)/libstratosphere/$(ATMOSPHERE_LIBRARY_DIR)/libstratosphere.a: check_lib
	@$(SILENTCMD)echo "Checked library."

check_lib:
	@$(MAKE) --no-print-directory -C $(ATMOSPHERE_LIBRARIES_DIR)/libstratosphere -f $(ATMOSPHERE_LIBRARIES_DIR)/libstratosphere/libstratosphere.mk

$(ATMOSPHERE_OUT_DIR) $(ATMOSPHERE_BUILD_DIR):
	@[ -d $@ ] || mkdir -p $@

#---------------------------------------------------------------------------------
clean:
	@echo clean ...
	@rm -fr $(BUILD) $(BOARD_TARGET) $(TARGET).elf
	@for i in $(ATMOSPHERE_OUT_DIR) $(ATMOSPHERE_BUILD_DIR); do [ -d $$i ] && rmdir $$i 2>/dev/null || true; done


#---------------------------------------------------------------------------------
else
.PHONY:	all

DEPENDS	:=	$(OFILES:.o=.d)

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
all	:	$(OUTPUT)$(BOARD_TARGET_SUFFIX)

%.kip : %.elf

%.nsp : %.nso %.npdm

%.nso: %.elf


#---------------------------------------------------------------------------------
$(OUTPUT).elf: $(OFILES) $(ATMOSPHERE_LIBRARIES_DIR)/libstratosphere/$(ATMOSPHERE_LIBRARY_DIR)/libstratosphere.a
	@echo linking $(notdir $@)
	$(SILENTCMD)$(LD) $(LDFLAGS) $(OFILES) $(LIBPATHS) $(LIBS) -o $@
	$(SILENTCMD)$(NM) -CSn $@ > $(notdir $(OUTPUT).lst)

$(OUTPUT).exe: $(OFILES) $(ATMOSPHERE_LIBRARIES_DIR)/libstratosphere/$(ATMOSPHERE_LIBRARY_DIR)/libstratosphere.a
	@echo linking $(notdir $@)
	$(SILENTCMD)$(LD) $(LDFLAGS) $(OFILES) $(LIBPATHS) $(LIBS) -o $@
	$(SILENTCMD)$(NM) -CSn $@ > $(notdir $*.lst)


ifeq ($(strip $(BOARD_TARGET_SUFFIX)),)
$(OUTPUT): $(OFILES) $(ATMOSPHERE_LIBRARIES_DIR)/libstratosphere/$(ATMOSPHERE_LIBRARY_DIR)/libstratosphere.a
	@echo linking $(notdir $@)
	$(SILENTCMD)$(LD) $(LDFLAGS) $(OFILES) $(LIBPATHS) $(LIBS) -o $@
	$(SILENTCMD)$(NM) -CSn $@ > $(notdir $@.lst)
endif

%.npdm  :   %.npdm.json
	@echo built ... $< $@
	@npdmtool $< $@
	@echo built ... $(notdir $@)

#---------------------------------------------------------------------------------
# you need a rule like this for each extension you use as binary data
#---------------------------------------------------------------------------------
%.bin.o	:	%.bin
#---------------------------------------------------------------------------------
	@echo $(notdir $<)
	@$(bin2o)

-include $(DEPENDS)

#---------------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------------
