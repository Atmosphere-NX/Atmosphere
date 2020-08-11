#---------------------------------------------------------------------------------
# Define the atmosphere board and cpu
#---------------------------------------------------------------------------------
export ATMOSPHERE_BOARD := nx-hac-001
export ATMOSPHERE_CPU   := arm-cortex-a57

#---------------------------------------------------------------------------------
# pull in common atmosphere configuration
#---------------------------------------------------------------------------------
include  $(dir $(abspath $(lastword $(MAKEFILE_LIST))))/../../libraries/config/templates/exosphere.mk

#---------------------------------------------------------------------------------
# no real need to edit anything past this point unless you need to add additional
# rules for different file extensions
#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#---------------------------------------------------------------------------------

export OUTPUT	:=	$(CURDIR)/$(TARGET)
export TOPDIR	:=	$(CURDIR)
export DEPSDIR  :=  $(CURDIR)/$(BUILD)

export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
			$(foreach dir,$(DATA),$(CURDIR)/$(dir)) \
			$(TOPDIR)/sc7fw \
			$(TOPDIR)/rebootstub

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

export LIBPATHS	:=	$(foreach dir,$(LIBDIRS),-L$(dir)/lib -L$(dir)/$(ATMOSPHERE_LIBRARY_DIR))

.PHONY: $(BUILD) clean all

#---------------------------------------------------------------------------------
all: $(BUILD) check_libexo

$(BUILD): check_libexo check_firmwares
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

check_libexo:
	@$(MAKE) --no-print-directory -C ../../libraries/libexosphere arm64

check_firmwares:
	@$(MAKE) -C $(TOPDIR)/sc7fw all
	@$(MAKE) -C $(TOPDIR)/rebootstub all

#---------------------------------------------------------------------------------
clean:
	@echo clean ...
	@$(MAKE) -C $(TOPDIR)/sc7fw clean
	@$(MAKE) -C $(TOPDIR)/rebootstub clean
	@rm -fr $(BUILD) $(OUTPUT).bin $(OUTPUT).elf *.lz4

#---------------------------------------------------------------------------------
else
.PHONY:	all

DEPENDS	:=	$(OFILES:.o=.d)

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
all	:	$(OUTPUT).lz4

$(OUTPUT).lz4	:	$(OUTPUT).bin
	@python ../split_program.py $(OUTPUT).bin $(dir $(OUTPUT))
	@echo built ... $(notdir $@)

$(OUTPUT).bin	:	$(OUTPUT).elf
	$(OBJCOPY) -S -O binary --set-section-flags .bss=alloc,load,contents $< $@
	@echo built ... $(notdir $@)

$(OUTPUT).elf	:	$(OFILES) ../../../libraries/libexosphere/$(ATMOSPHERE_LIBRARY_DIR)/libexosphere.a

secmon_crt0_cpp.o secmon_make_page_table.o : CFLAGS += -fno-builtin

%.elf:
	@echo linking $(notdir $@)
	$(LD) $(LDFLAGS) $(OFILES) $(LIBPATHS) $(LIBS) -o $@
	@$(NM) -CSn $@ > $(notdir $*.lst)

$(OFILES_SRC)	: $(HFILES_BIN)

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
