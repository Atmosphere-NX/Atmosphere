#---------------------------------------------------------------------------------
# pull in common atmosphere configuration
#---------------------------------------------------------------------------------
THIS_MAKEFILE     := $(abspath $(lastword $(MAKEFILE_LIST)))
CURRENT_DIRECTORY := $(abspath $(dir $(THIS_MAKEFILE)))
include $(CURRENT_DIRECTORY)/../config/common.mk

#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------
PRECOMPILED_HEADERS :=  include/mesosphere.hpp
#PRECOMPILED_HEADERS :=

ifeq ($(ATMOSPHERE_BUILD_FOR_DEBUGGING),1)
ATMOSPHERE_OPTIMIZATION_FLAG := -Os
else
ATMOSPHERE_OPTIMIZATION_FLAG := -O3
endif

DEFINES	    := $(ATMOSPHERE_DEFINES) -DATMOSPHERE_IS_MESOSPHERE
SETTINGS    := $(ATMOSPHERE_SETTINGS) $(ATMOSPHERE_OPTIMIZATION_FLAG) -mgeneral-regs-only -ffixed-x18 -Wextra -Werror -fno-non-call-exceptions
CFLAGS      := $(ATMOSPHERE_CFLAGS) $(SETTINGS) $(DEFINES) $(INCLUDE)
CXXFLAGS    := $(CFLAGS) $(ATMOSPHERE_CXXFLAGS) -fno-use-cxa-atexit
ASFLAGS     := $(ATMOSPHERE_ASFLAGS) $(SETTINGS) $(DEFINES) $(INCLUDE)

SOURCES     += $(foreach v,$(call ALL_SOURCE_DIRS,../libvapours/source),$(if $(findstring ../libvapours/source/sdmmc,$v),,$v))

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

.PHONY: clean all

all: $(ATMOSPHERE_LIBRARY_DIR) $(ATMOSPHERE_BUILD_DIR) $(SOURCES) $(INCLUDES) $(GCH_DIRS) $(CPPFILES) $(CFILES) $(SFILES)
	@$(MAKE) __RECURSIVE__=1 OUTPUT=$(CURDIR)/$(ATMOSPHERE_LIBRARY_DIR)/$(TARGET).a \
	DEPSDIR=$(CURDIR)/$(ATMOSPHERE_BUILD_DIR) \
	--no-print-directory -C $(ATMOSPHERE_BUILD_DIR) \
	-f $(THIS_MAKEFILE)

#---------------------------------------------------------------------------------
clean:
	@echo clean $(ATMOSPHERE_BUILD_NAME) ...
	@rm -fr $(ATMOSPHERE_BUILD_DIR) $(ATMOSPHERE_OUT_DIR)
	@rm -fr $(foreach hdr,$(GCH_DIRS),$(hdr)/$(ATMOSPHERE_GCH_IDENTIFIER))
	@for i in $(GCH_DIRS); do [ -d $$i ] && rmdir --ignore-fail-on-non-empty $$i || true; done

$(ATMOSPHERE_LIBRARY_DIR) $(ATMOSPHERE_BUILD_DIR) $(GCH_DIRS):
	@[ -d $@ ] || mkdir -p $@

#---------------------------------------------------------------------------------
else

GCH_FILES := $(foreach hdr,$(PRECOMPILED_HEADERS:.hpp=.hpp.gch),$(CURRENT_DIRECTORY)/$(hdr)/$(ATMOSPHERE_GCH_IDENTIFIER))
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

