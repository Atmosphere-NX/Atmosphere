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
			$(foreach dir,$(DATA),$(CURDIR)/$(dir)) $(CURRENT_DIRECTORY)/../program/$(ATMOSPHERE_OUT_DIR)

CFILES      :=	$(call FIND_SOURCE_FILES,$(SOURCES),c)
CPPFILES    :=	$(call FIND_SOURCE_FILES,$(SOURCES),cpp)
SFILES      :=	$(call FIND_SOURCE_FILES,$(SOURCES),s)
BINFILES    := program.lz4

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

.PHONY: clean all check_lib

#---------------------------------------------------------------------------------
all: $(ATMOSPHERE_OUT_DIR) $(ATMOSPHERE_BUILD_DIR) $(ATMOSPHERE_LIBRARIES_DIR)/libexosphere/$(ATMOSPHERE_LIBRARY_DIR)/libexosphere.a $(CURRENT_DIRECTORY)/../program/$(ATMOSPHERE_OUT_DIR)/program.lz4
	@$(MAKE) __RECURSIVE__=1 OUTPUT=$(CURRENT_DIRECTORY)/$(ATMOSPHERE_OUT_DIR)/$(notdir $(ATMOSPHERE_TOPDIR)) \
	DEPSDIR=$(CURDIR)/$(ATMOSPHERE_BUILD_DIR) \
	--no-print-directory -C $(ATMOSPHERE_BUILD_DIR) \
	-f $(THIS_MAKEFILE)

$(ATMOSPHERE_LIBRARIES_DIR)/libexosphere/$(ATMOSPHERE_LIBRARY_DIR)/libexosphere.a: check_lib
	@$(SILENTCMD)echo "Checked library."

$(CURRENT_DIRECTORY)/../program/$(ATMOSPHERE_OUT_DIR)/program.lz4: check_fusee_program
	@$(SILENTCMD)echo "Checked fusee program."

ifeq ($(ATMOSPHERE_CHECKED_LIBEXOSPHERE),1)
check_lib:
else
check_lib:
	@$(MAKE) --no-print-directory -C $(ATMOSPHERE_LIBRARIES_DIR)/libexosphere -f $(ATMOSPHERE_LIBRARIES_DIR)/libexosphere/libexosphere.mk
endif

ifeq ($(ATMOSPHERE_CHECKED_FUSEE_PROGRAM),1)
check_fusee_program:
else
check_fusee_program: check_lib
	@$(MAKE) --no-print-directory -C $(CURRENT_DIRECTORY)/../program -f $(CURRENT_DIRECTORY)/../program/program.mk ATMOSPHERE_CHECKED_LIBEXOSPHERE=1
endif

$(ATMOSPHERE_OUT_DIR) $(ATMOSPHERE_BUILD_DIR):
	@[ -d $@ ] || mkdir -p $@

#---------------------------------------------------------------------------------
clean:
	@echo clean ...
	@rm -fr $(ATMOSPHERE_BUILD_DIR) $(ATMOSPHERE_OUT_DIR)

#---------------------------------------------------------------------------------
else

DEPENDS	:=	$(OFILES:.o=.d)

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------

$(OUTPUT).bin	:	$(OUTPUT).elf
	$(OBJCOPY) -S -O binary --set-section-flags .bss=alloc,load,contents $< $@
	@echo built ... $(notdir $@)

$(OUTPUT).elf	:	$(OFILES)

$(OFILES)	:	 $(ATMOSPHERE_LIBRARIES_DIR)/libexosphere/$(ATMOSPHERE_LIBRARY_DIR)/libexosphere.a

%.elf:
	@echo linking $(notdir $@)
	$(LD) $(LDFLAGS) $(OFILES) $(LIBPATHS) $(LIBS) -o $@
	@$(NM) -CSn $@ > $(notdir $*.lst)

program.lz4.o: program.lz4
	@echo $(notdir $<)
	@$(bin2o)

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
