#---------------------------------------------------------------------------------
# pull in common atmosphere configuration
#---------------------------------------------------------------------------------
THIS_MAKEFILE     := $(abspath $(lastword $(MAKEFILE_LIST)))
CURRENT_DIRECTORY := $(abspath $(dir $(THIS_MAKEFILE)))
include $(CURRENT_DIRECTORY)/../config/common.mk

ifeq ($(ATMOSPHERE_BOARD),nx-hac-001)

#---------------------------------------------------------------------------------
# pull in switch rules
#---------------------------------------------------------------------------------
ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>/devkitpro")
endif

include $(DEVKITPRO)/libnx/switch_rules

endif

#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------
ifeq ($(strip $(ATMOSPHERE_COMPILER_NAME)),gcc)
PRECOMPILED_HEADERS := include/stratosphere.hpp
else
PRECOMPILED_HEADERS :=
endif

ifeq ($(ATMOSPHERE_BUILD_FOR_DEBUGGING),1)
ATMOSPHERE_OPTIMIZATION_FLAG := -Os
else
ATMOSPHERE_OPTIMIZATION_FLAG := -O2
endif

DEFINES	    := $(ATMOSPHERE_DEFINES) -DATMOSPHERE_IS_STRATOSPHERE -D_GNU_SOURCE
SETTINGS    := $(ATMOSPHERE_SETTINGS) $(ATMOSPHERE_OPTIMIZATION_FLAG) -Wextra -Werror -Wno-missing-field-initializers -flto
CFLAGS      := $(ATMOSPHERE_CFLAGS) $(SETTINGS) $(DEFINES) $(INCLUDE)
CXXFLAGS    := $(CFLAGS) $(ATMOSPHERE_CXXFLAGS)
ASFLAGS     := $(ATMOSPHERE_ASFLAGS) $(SETTINGS) $(DEFINES)

ifeq ($(ATMOSPHERE_BOARD),nx-hac-001)
LDFLAGS     := -specs=$(DEVKITPRO)/libnx/switch.specs $(SETTINGS) -Wl,-Map,$(notdir $*.map)
else
LDFLAGS     := -Wl,-Map,$(notdir $*.map)
endif


SOURCES     += $(call ALL_SOURCE_DIRS,../libvapours/source)
SOURCES     += $(call UNFILTERED_SOURCE_DIRS,source/os)

ifeq ($(ATMOSPHERE_BOARD),nx-hac-001)
LIBS        := -lnx
else
LIBS        :=
endif

#---------------------------------------------------------------------------------
# list of directories containing libraries, this must be the top level containing
# include and lib
#---------------------------------------------------------------------------------
ifeq ($(ATMOSPHERE_BOARD),nx-hac-001)
LIBDIRS	:= $(PORTLIBS) $(LIBNX) $(ATMOSPHERE_LIBDIRS) $(ATMOSPHERE_LIBRARIES_DIR)/libvapours
else
LIBDIRS	:= $(ATMOSPHERE_LIBDIRS) $(ATMOSPHERE_LIBRARIES_DIR)/libvapours
endif

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
	@for i in $(GCH_DIRS); do [ -d $$i ] && rmdir $$i 2>/dev/null || true; done

$(ATMOSPHERE_LIBRARY_DIR) $(ATMOSPHERE_BUILD_DIR) $(GCH_DIRS):
	@[ -d $@ ] || mkdir -p $@

#---------------------------------------------------------------------------------
else

GCH_FILES := $(foreach hdr,$(PRECOMPILED_HEADERS:.hpp=.hpp.gch),$(CURRENT_DIRECTORY)/$(hdr)/$(ATMOSPHERE_GCH_IDENTIFIER))
DEPENDS	:=	$(OFILES:.o=.d) $(foreach hdr,$(GCH_FILES),$(notdir $(patsubst %.hpp.gch/,%.d,$(dir $(hdr)))))

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
$(OUTPUT)	:	result_get_name.o $(filter-out result_get_name.o, $(OFILES))

$(filter-out result_get_name.o, $(OFILES))	:	$(GCH_FILES)

$(OFILES_SRC)	: $(HFILES_BIN)

ams_environment_weak.os.horizon.o: CXXFLAGS += -fno-lto
hos_version_api_weak_for_unit_test.o: CXXFLAGS += -fno-lto
pm_info_api_weak.o: CXXFLAGS += -fno-lto
hos_stratosphere_api.o: CXXFLAGS += -fno-lto

init_operator_new.o: CXXFLAGS += -fno-lto
init_libnx_shim.os.horizon.o: CXXFLAGS += -fno-lto

result_get_name.o: CXXFLAGS += -fno-lto

crypto_sha256_generator.o: CXXFLAGS += -fno-lto

spl_secure_monitor_api.os.generic.o: CXXFLAGS += -I$(ATMOSPHERE_LIBRARIES_DIR)/libexosphere/include
fs_id_string_impl.os.generic.o: CXXFLAGS += -I$(ATMOSPHERE_LIBRARIES_DIR)/libexosphere/include

ifeq ($(ATMOSPHERE_OS_NAME),windows)
# Audit builds fail when these have lto disabled.
# Noting 10/29/24:
# In member function '__ct ':
# internal compiler error: in binds_to_current_def_p, at symtab.cc:2589
os_%.o: CXXFLAGS += -fno-lto
fssystem_%.o: CXXFLAGS += -fno-lto
fssrv_%.o: CXXFLAGS += -fno-lto
fs_%.o: CXXFLAGS += -fno-lto
endif

#---------------------------------------------------------------------------------
%_bin.h %.bin.o	:	%.bin
#---------------------------------------------------------------------------------
	@echo $(notdir $<)
	@$(bin2o)


-include $(DEPENDS)

#---------------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------------

