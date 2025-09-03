#---------------------------------------------------------------------------------
# pull in common atmosphere configuration
#---------------------------------------------------------------------------------
THIS_MAKEFILE     := $(abspath $(lastword $(MAKEFILE_LIST)))
CURRENT_DIRECTORY := $(abspath $(dir $(THIS_MAKEFILE)))
include $(CURRENT_DIRECTORY)/../../libraries/config/templates/exosphere.mk

#---------------------------------------------------------------------------------
# no real need to edit anything past this point unless you need to add additional
# rules for different file extensions
#---------------------------------------------------------------------------------
ifneq ($(__RECURSIVE__),1)
#---------------------------------------------------------------------------------

export ATMOSPHERE_TOPDIR := $(CURRENT_DIRECTORY)

export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) $(CURDIR)/include \
			$(foreach dir,$(DATA),$(CURDIR)/$(dir)) \
            $(CURRENT_DIRECTORY)/sc7fw/$(ATMOSPHERE_BOOT_OUT_DIR) \
            $(CURRENT_DIRECTORY)/rebootstub/$(ATMOSPHERE_BOOT_OUT_DIR)

CFILES      :=	$(call FIND_SOURCE_FILES,$(SOURCES),c)
CPPFILES    :=	$(call FIND_SOURCE_FILES,$(SOURCES),cpp)
SFILES      :=	$(call FIND_SOURCE_FILES,$(SOURCES),s)
BINFILES    := sc7fw.bin rebootstub.bin

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

export OFILES_BIN	:=	$(addsuffix .o,$(BINFILES))
export OFILES_SRC	:=	$(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)
export OFILES 	:=	$(OFILES_BIN) $(OFILES_SRC)
export HFILES_BIN	:=	$(addsuffix .h,$(subst .,_,$(subst -,_,$(BINFILES))))

export INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
			$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
			-I.

export LIBPATHS	:=	$(foreach dir,$(LIBDIRS),-L$(dir)/$(ATMOSPHERE_LIBRARY_DIR))

.PHONY: clean all check_lib check_boot_lib check_sc7fw check_rebootstub

#---------------------------------------------------------------------------------
all: $(ATMOSPHERE_OUT_DIR) $(ATMOSPHERE_BUILD_DIR) $(ATMOSPHERE_LIBRARIES_DIR)/libexosphere/$(ATMOSPHERE_LIBRARY_DIR)/libexosphere.a $(CURRENT_DIRECTORY)/sc7fw/$(ATMOSPHERE_BOOT_OUT_DIR)/sc7fw.bin $(CURRENT_DIRECTORY)/rebootstub/$(ATMOSPHERE_BOOT_OUT_DIR)/rebootstub.bin
	@$(MAKE) __RECURSIVE__=1 OUTPUT=$(CURRENT_DIRECTORY)/$(ATMOSPHERE_OUT_DIR)/$(notdir $(ATMOSPHERE_TOPDIR)) \
	DEPSDIR=$(CURDIR)/$(ATMOSPHERE_BUILD_DIR) \
	--no-print-directory -C $(ATMOSPHERE_BUILD_DIR) \
	-f $(THIS_MAKEFILE)

$(ATMOSPHERE_LIBRARIES_DIR)/libexosphere/$(ATMOSPHERE_LIBRARY_DIR)/libexosphere.a: check_lib
	@$(SILENTCMD)echo "Checked library."


ifneq ($(strip $(ATMOSPHERE_LIBRARY_DIR)),$(strip $(ATMOSPHERE_BOOT_LIBRARY_DIR)))
$(ATMOSPHERE_LIBRARIES_DIR)/libexosphere/$(ATMOSPHERE_BOOT_LIBRARY_DIR)/libexosphere.a: check_boot_lib
	@$(SILENTCMD)echo "Checked boot library."
endif

$(CURRENT_DIRECTORY)/sc7fw/$(ATMOSPHERE_BOOT_OUT_DIR)/sc7fw.bin: check_sc7fw

$(CURRENT_DIRECTORY)/rebootstub/$(ATMOSPHERE_BOOT_OUT_DIR)/rebootstub.bin: check_rebootstub

ifeq ($(ATMOSPHERE_CHECKED_SC7FW),1)
check_sc7fw:
else
check_sc7fw: $(ATMOSPHERE_LIBRARIES_DIR)/libexosphere/$(ATMOSPHERE_BOOT_LIBRARY_DIR)/libexosphere.a
	@$(MAKE) --no-print-directory -C $(CURRENT_DIRECTORY)/sc7fw -f $(CURRENT_DIRECTORY)/sc7fw/sc7fw.mk ATMOSPHERE_CPU="$(strip $(ATMOSPHERE_BOOT_CPU))" ATMOSPHERE_CHECKED_LIBEXOSPHERE=1 ATMOSPHERE_CHECKED_BOOT_LIBEXOSPHERE=1
endif

ifeq ($(ATMOSPHERE_CHECKED_REBOOTSTUB),1)
check_rebootstub:
else
check_rebootstub: $(ATMOSPHERE_LIBRARIES_DIR)/libexosphere/$(ATMOSPHERE_BOOT_LIBRARY_DIR)/libexosphere.a
	@$(MAKE) --no-print-directory -C $(CURRENT_DIRECTORY)/rebootstub -f $(CURRENT_DIRECTORY)/rebootstub/rebootstub.mk ATMOSPHERE_CPU="$(strip $(ATMOSPHERE_BOOT_CPU))" ATMOSPHERE_CHECKED_LIBEXOSPHERE=1 ATMOSPHERE_CHECKED_BOOT_LIBEXOSPHERE=1
endif

ifeq ($(ATMOSPHERE_CHECKED_LIBEXOSPHERE),1)
check_lib:
else
check_lib:
	@$(MAKE) --no-print-directory -C $(ATMOSPHERE_LIBRARIES_DIR)/libexosphere -f $(ATMOSPHERE_LIBRARIES_DIR)/libexosphere/libexosphere.mk
endif

ifeq ($(ATMOSPHERE_CHECKED_BOOT_LIBEXOSPHERE),1)
check_boot_lib:
else
check_boot_lib:
	@$(MAKE) --no-print-directory -C $(ATMOSPHERE_LIBRARIES_DIR)/libexosphere -f $(ATMOSPHERE_LIBRARIES_DIR)/libexosphere/libexosphere.mk ATMOSPHERE_CPU="$(strip $(ATMOSPHERE_BOOT_CPU))"
endif

$(ATMOSPHERE_OUT_DIR) $(ATMOSPHERE_BUILD_DIR):
	@[ -d $@ ] || mkdir -p $@

#---------------------------------------------------------------------------------
clean:
	@echo clean ...
	@$(MAKE) --no-print-directory -C $(CURRENT_DIRECTORY)/rebootstub -f $(CURRENT_DIRECTORY)/rebootstub/rebootstub.mk clean ATMOSPHERE_CPU="$(strip $(ATMOSPHERE_BOOT_CPU))"
	@$(MAKE) --no-print-directory -C $(CURRENT_DIRECTORY)/sc7fw -f $(CURRENT_DIRECTORY)/sc7fw/sc7fw.mk clean ATMOSPHERE_CPU="$(strip $(ATMOSPHERE_BOOT_CPU))"
	@rm -fr $(ATMOSPHERE_BUILD_DIR) $(ATMOSPHERE_OUT_DIR)

#---------------------------------------------------------------------------------
else

DEPENDS	:=	$(OFILES:.o=.d)

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------

$(OUTPUT).lz4	:	$(OUTPUT).bin
	$(SILENTCMD)$(PYTHON) $(CURRENT_DIRECTORY)/split_program.py $(OUTPUT).bin $(CURRENT_DIRECTORY)/$(ATMOSPHERE_OUT_DIR)
	@echo built ... $(notdir $@)

$(OUTPUT).bin	:	$(OUTPUT).elf
	$(OBJCOPY) -S -O binary --set-section-flags .bss=alloc,load,contents $< $@
	@echo built ... $(notdir $@)

$(OUTPUT).elf	:	$(OFILES)

$(OFILES)	:	 $(ATMOSPHERE_LIBRARIES_DIR)/libexosphere/$(ATMOSPHERE_LIBRARY_DIR)/libexosphere.a

secmon_crt0_cpp.o secmon_make_page_table.o : CFLAGS += -fno-builtin

sc7fw.bin.o: sc7fw.bin
	@echo $(notdir $<)
	@$(bin2o)

rebootstub.bin.o: rebootstub.bin
	@echo $(notdir $<)
	@$(bin2o)

%.elf:
	@echo linking $(notdir $@)
	$(LD) $(LDFLAGS) $(OFILES) $(LIBPATHS) $(LIBS) -o $@
	@$(NM) -CSn $@ > $(notdir $*.lst)

$(OFILES_SRC)	: $(OFILES_BIN)

#---------------------------------------------------------------------------------
# you need a rule like this for each extension you use as binary data
#---------------------------------------------------------------------------------
%.bin.o	%_bin.h:	%.bin
#---------------------------------------------------------------------------------
	@echo $(notdir $<)
	@$(bin2o)

-include $(DEPENDS)

#---------------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------------
