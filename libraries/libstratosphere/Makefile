#---------------------------------------------------------------------------------
# pull in common atmosphere configuration
#---------------------------------------------------------------------------------
THIS_MAKEFILE     := $(abspath $(lastword $(MAKEFILE_LIST)))
CURRENT_DIRECTORY := $(abspath $(dir $(THIS_MAKEFILE)))
include $(CURRENT_DIRECTORY)/../config/common.mk

#---------------------------------------------------------------------------------
# pull in switch rules
#---------------------------------------------------------------------------------
ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>/devkitpro")
endif

include $(DEVKITPRO)/libnx/switch_rules

#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------
PRECOMPILED_HEADERS := $(CURRENT_DIRECTORY)/include/stratosphere.hpp

DEFINES	    := $(ATMOSPHERE_DEFINES) -DATMOSPHERE_IS_STRATOSPHERE -D_GNU_SOURCE
SETTINGS    := $(ATMOSPHERE_SETTINGS) -O2
CFLAGS      := $(ATMOSPHERE_CFLAGS) $(SETTINGS) $(DEFINES) $(INCLUDE)
CXXFLAGS    := $(CFLAGS) $(ATMOSPHERE_CXXFLAGS) -flto
ASFLAGS     := $(ATMOSPHERE_ASFLAGS) $(SETTINGS)

LDFLAGS     := -specs=$(DEVKITPRO)/libnx/switch.specs $(SETTINGS) -Wl,-Map,$(notdir $*.map)

SOURCES     += $(call ALL_SOURCE_DIRS,../libvapours/source)
SOURCES     += $(call UNFILTERED_SOURCE_DIRS,source/os)

LIBS        := -lnx

#---------------------------------------------------------------------------------
# list of directories containing libraries, this must be the top level containing
# include and lib
#---------------------------------------------------------------------------------
LIBDIRS	:= $(PORTLIBS) $(LIBNX) $(ATMOSPHERE_LIBRARIES_DIR)/libvapours

#---------------------------------------------------------------------------------
# no real need to edit anything past this point unless you need to add additional
# rules for different file extensions
#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
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
export GCH_FILES    :=	$(PRECOMPILED_HEADERS:.hpp=.hpp.gch)
export OFILES 	:=	$(OFILES_BIN) $(OFILES_SRC)
export HFILES_BIN	:=	$(addsuffix .h,$(subst .,_,$(BINFILES)))

export INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
			$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
			-I.

.PHONY: clean all

#---------------------------------------------------------------------------------
all: lib/$(TARGET).a

lib:
	@[ -d $@ ] || mkdir -p $@

release:
	@[ -d $@ ] || mkdir -p $@

lib/$(TARGET).a : lib release $(SOURCES) $(INCLUDES)
	@$(MAKE) BUILD=release OUTPUT=$(CURDIR)/$@ \
	BUILD_CFLAGS="-DNDEBUG=1 -O2" \
	DEPSDIR=$(CURDIR)/release \
	--no-print-directory -C release \
	-f $(CURDIR)/Makefile

dist-bin: all
	@tar --exclude=*~ -cjf $(TARGET).tar.bz2 include lib

dist-src:
	@tar --exclude=*~ -cjf $(TARGET)-src.tar.bz2 include source Makefile

dist: dist-src dist-bin

#---------------------------------------------------------------------------------
clean:
	@echo clean ...
	@rm -fr release lib *.bz2 $(GCH_FILES)

#---------------------------------------------------------------------------------
else

DEPENDS	:=	$(OFILES:.o=.d) $(foreach hdr,$(GCH_FILES:.hpp.gch=.d),$(notdir $(hdr)))

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
$(OUTPUT)	:	$(OFILES)

$(OFILES)	:	$(GCH_FILES)

$(OFILES_SRC)	: $(HFILES_BIN)

#---------------------------------------------------------------------------------
%_bin.h %.bin.o	:	%.bin
#---------------------------------------------------------------------------------
	@echo $(notdir $<)
	@$(bin2o)


-include $(DEPENDS)

#---------------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------------

