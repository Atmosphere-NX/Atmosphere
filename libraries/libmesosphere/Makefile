#---------------------------------------------------------------------------------
# pull in common atmosphere configuration
#---------------------------------------------------------------------------------
THIS_MAKEFILE     := $(abspath $(lastword $(MAKEFILE_LIST)))
CURRENT_DIRECTORY := $(abspath $(dir $(THIS_MAKEFILE)))
include $(CURRENT_DIRECTORY)/../config/common.mk

#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------
PRECOMPILED_HEADERS := include/mesosphere.hpp

DEFINES     := $(ATMOSPHERE_DEFINES) -DATMOSPHERE_IS_MESOSPHERE
SETTINGS    := $(ATMOSPHERE_SETTINGS) -Os -mgeneral-regs-only -ffixed-x18 -Wextra -Werror -fno-non-call-exceptions
CFLAGS      := $(ATMOSPHERE_CFLAGS) $(SETTINGS) $(DEFINES) $(INCLUDE)
CXXFLAGS    := $(CFLAGS) $(ATMOSPHERE_CXXFLAGS) -fno-use-cxa-atexit -flto
ASFLAGS     := $(ATMOSPHERE_ASFLAGS) $(SETTINGS) $(DEFINES) $(INCLUDE)

SOURCES     += $(call ALL_SOURCE_DIRS,../libvapours/source)

LIBS        :=

#---------------------------------------------------------------------------------
# list of directories containing libraries, this must be the top level containing
# include and lib
#---------------------------------------------------------------------------------
LIBDIRS	:= $(ATMOSPHERE_LIBRARIES_DIR)/libvapours

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
export GCH_DIRS     :=  $(PRECOMPILED_HEADERS:.hpp=.hpp.gch)
export OFILES 	:=	$(OFILES_BIN) $(OFILES_SRC)
export HFILES_BIN	:=	$(addsuffix .h,$(subst .,_,$(BINFILES)))

export INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
			$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
			-I.

#---------------------------------------------------------------------------------

ATMOSPHERE_BUILD_CONFIGS :=
all: release

define ATMOSPHERE_ADD_TARGET

ATMOSPHERE_BUILD_CONFIGS += $(strip $1)

$(strip $1): $$(ATMOSPHERE_LIBRARY_DIR)/$(strip $2)

$$(ATMOSPHERE_LIBRARY_DIR)/$(strip $2) : $$(ATMOSPHERE_LIBRARY_DIR) $$(ATMOSPHERE_BUILD_DIR)/$(strip $1) $$(SOURCES) $$(INCLUDES) $$(GCH_DIRS)
	@$$(MAKE) __RECURSIVE__=1 OUTPUT=$$(CURDIR)/$$@ $(3) \
	ATMOSPHERE_GCH_IDENTIFIER="$$(ATMOSPHERE_BOARD_NAME)_$$(ATMOSPHERE_ARCH_NAME)_$(strip $1)" \
	DEPSDIR=$$(CURDIR)/$$(ATMOSPHERE_BUILD_DIR)/$(strip $1) \
	--no-print-directory -C $$(ATMOSPHERE_BUILD_DIR)/$(strip $1) \
	-f $$(THIS_MAKEFILE)

clean-$(strip $1):
	@echo clean $(strip $1) ...
	@rm -fr $$(ATMOSPHERE_BUILD_DIR)/$(strip $1) $$(ATMOSPHERE_LIBRARY_DIR)/$(strip $2)
	@rm -fr $$(foreach hdr,$$(GCH_DIRS),$$(hdr)/$$(ATMOSPHERE_BOARD_NAME)_$$(ATMOSPHERE_ARCH_NAME)_$(strip $1))
	@for i in $$(GCH_DIRS) $$(ATMOSPHERE_BUILD_DIR) $$(ATMOSPHERE_LIBRARY_DIR); do [ -d $$$$i ] && rmdir --ignore-fail-on-non-empty $$$$i || true; done

endef

$(eval $(call ATMOSPHERE_ADD_TARGET, release, $(TARGET).a, \
	ATMOSPHERE_BUILD_SETTINGS="" \
))

$(eval $(call ATMOSPHERE_ADD_TARGET, debug, $(TARGET)_debug.a, \
	ATMOSPHERE_BUILD_SETTINGS="-DMESOSPHERE_BUILD_FOR_DEBUGGING" \
))

$(eval $(call ATMOSPHERE_ADD_TARGET, audit, $(TARGET)_audit.a, \
	ATMOSPHERE_BUILD_SETTINGS="-DMESOSPHERE_BUILD_FOR_AUDITING" \
))

#---------------------------------------------------------------------------------

-include $(ATMOSPHERE_BOARD_NAME)_$(ATMOSPHERE_ARCH_NAME).mk

ALL_GCH_IDENTIFIERS := $(foreach config,$(ATMOSPHERE_BUILD_CONFIGS),$(ATMOSPHERE_BOARD_NAME)_$(ATMOSPHERE_ARCH_NAME)_$(config))
ALL_GCH_FILES       := $(foreach hdr,$(PRECOMPILED_HEADERS:.hpp=.hpp.gch),$(foreach id,$(ALL_GCH_IDENTIFIERS),$(hdr)/$(id)))

.PHONY: clean all $(foreach config,$(ATMOSPHERE_BUILD_CONFIGS),$(config) clean-$(config))

$(ATMOSPHERE_LIBRARY_DIR) $(GCH_DIRS):
	@[ -d $@ ] || mkdir -p $@

$(ATMOSPHERE_BUILD_DIR)/%:
	@[ -d $@ ] || mkdir -p $@

#---------------------------------------------------------------------------------
clean:
	@echo clean ...
	@rm -fr $(ATMOSPHERE_BUILD_DIR) $(ATMOSPHERE_LIBRARY_DIR) *.bz2 $(ALL_GCH_FILES)
	@for i in $(GCH_DIRS); do [ -d $$i ] && rmdir --ignore-fail-on-non-empty $$i || true; done

#---------------------------------------------------------------------------------
else

GCH_FILES :=	$(foreach hdr,$(PRECOMPILED_HEADERS:.hpp=.hpp.gch),$(CURRENT_DIRECTORY)/$(hdr)/$(ATMOSPHERE_GCH_IDENTIFIER))
DEPENDS	:=	$(OFILES:.o=.d) $(foreach hdr,$(GCH_FILES),$(notdir $(patsubst %.hpp.gch/,%.d,$(dir $(hdr)))))

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
$(OUTPUT)	:	$(OFILES)

$(filter-out kern_svc_tables.o, $(OFILES))	:	$(GCH_FILES)

$(OFILES_SRC)	: $(HFILES_BIN)

kern_libc_generic.o: CFLAGS += -fno-builtin

#---------------------------------------------------------------------------------
%_bin.h %.bin.o	:	%.bin
#---------------------------------------------------------------------------------
	@echo $(notdir $<)
	@$(bin2o)


-include $(DEPENDS)

#---------------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------------

