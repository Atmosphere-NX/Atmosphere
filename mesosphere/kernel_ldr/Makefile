#---------------------------------------------------------------------------------
# pull in common atmosphere configuration
#---------------------------------------------------------------------------------
THIS_MAKEFILE     := $(abspath $(lastword $(MAKEFILE_LIST)))
CURRENT_DIRECTORY := $(abspath $(dir $(THIS_MAKEFILE)))
include $(CURRENT_DIRECTORY)/../../libraries/config/templates/mesosphere.mk

#---------------------------------------------------------------------------------
# no real need to edit anything past this point unless you need to add additional
# rules for different file extensions
#---------------------------------------------------------------------------------
ifneq ($(__RECURSIVE__),1)
#---------------------------------------------------------------------------------

export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) $(CURDIR)/include \
			$(foreach dir,$(DATA),$(CURDIR)/$(dir))

CFILES      :=	$(call FIND_SOURCE_FILES,$(SOURCES),c)
CPPFILES    :=	$(call FIND_SOURCE_FILES,$(SOURCES),cpp)
SFILES      :=	$(call FIND_SOURCE_FILES,$(SOURCES),s)

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

export TOPDIR := $(CURRENT_DIRECTORY)

OUTPUT_BASE := $(TOPDIR)/$(notdir $(TOPDIR))

#---------------------------------------------------------------------------------

ATMOSPHERE_BUILD_CONFIGS :=
all: release

define ATMOSPHERE_ADD_TARGET

ATMOSPHERE_BUILD_CONFIGS += $(strip $1)

$(strip $1): check_libmeso_$(strip $1) $$(ATMOSPHERE_BUILD_DIR)/$(strip $1)
	@$$(MAKE) __RECURSIVE__=1 OUTPUT=$$(OUTPUT_BASE)$(strip $2) $(3) \
	DEPSDIR=$$(CURDIR)/$$(ATMOSPHERE_BUILD_DIR)/$(strip $1) \
	LIBMESOSPHERE_NAME=mesosphere$(strip $2) \
	--no-print-directory -C $$(ATMOSPHERE_BUILD_DIR)/$(strip $1) \
	-f $$(THIS_MAKEFILE)

check_libmeso_$(strip $1):
	@$$(MAKE) --no-print-directory -C ../../libraries/libmesosphere $(strip $1)

clean-$(strip $1):
	@echo clean $(strip $1) ...
	@rm -fr $$(ATMOSPHERE_BUILD_DIR)/$(strip $1) $$(OUTPUT_BASE)$(strip $2).bin $$(OUTPUT_BASE)$(strip $2).elf

endef

$(eval $(call ATMOSPHERE_ADD_TARGET, release,     , \
	ATMOSPHERE_BUILD_SETTINGS="" \
))

$(eval $(call ATMOSPHERE_ADD_TARGET, debug, _debug, \
	ATMOSPHERE_BUILD_SETTINGS="-DMESOSPHERE_BUILD_FOR_DEBUGGING" \
))

$(eval $(call ATMOSPHERE_ADD_TARGET, audit, _audit, \
	ATMOSPHERE_BUILD_SETTINGS="-DMESOSPHERE_BUILD_FOR_AUDITING" \
))

$(ATMOSPHERE_BUILD_DIR)/%:
	@[ -d $@ ] || mkdir -p $@

#---------------------------------------------------------------------------------
clean: $(foreach config,$(ATMOSPHERE_BUILD_CONFIGS),clean-$(config))

.PHONY: all clean $(foreach config,$(ATMOSPHERE_BUILD_CONFIGS),$(config) clean-$(config))

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

$(OFILES)	:	$(ATMOSPHERE_LIBRARIES_DIR)/libmesosphere/$(ATMOSPHERE_LIBRARY_DIR)/lib$(LIBMESOSPHERE_NAME).a

%.elf:
	@echo linking $(notdir $@)
	$(LD) $(LDFLAGS) $(OFILES) $(LIBPATHS) $(LIBS) -o $@
	@$(NM) -CSn $@ > $(notdir $*.lst)

$(OFILES_SRC)	: $(HFILES_BIN)

kern_libc_generic.o: CFLAGS += -fno-builtin

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