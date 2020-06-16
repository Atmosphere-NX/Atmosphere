#---------------------------------------------------------------------------------
# Define the atmosphere board and cpu
#---------------------------------------------------------------------------------
export ATMOSPHERE_BOARD := nx-hac-001
export ATMOSPHERE_CPU   := arm7tdmi

#---------------------------------------------------------------------------------
# pull in common atmosphere configuration
#---------------------------------------------------------------------------------
include  $(dir $(abspath $(lastword $(MAKEFILE_LIST))))/../config/common.mk

#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------

DEFINES     := $(ATMOSPHERE_DEFINES) -DATMOSPHERE_IS_EXOSPHERE
SETTINGS    := $(ATMOSPHERE_SETTINGS) -Os -Werror -flto -fno-non-call-exceptions
CFLAGS      := $(ATMOSPHERE_CFLAGS) $(SETTINGS) $(DEFINES) $(INCLUDE)
CXXFLAGS    := $(CFLAGS) $(ATMOSPHERE_CXXFLAGS) -fno-use-cxa-atexit
ASFLAGS     := $(ATMOSPHERE_ASFLAGS) $(SETTINGS)

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
ifneq ($(BUILD),$(notdir $(CURDIR)))
#---------------------------------------------------------------------------------

export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) $(CURDIR)/include \
			$(foreach dir,$(DATA),$(CURDIR)/$(dir))

CFILES      :=	$(foreach dir,$(SOURCES),$(filter-out $(notdir $(wildcard $(dir)/*.arch.*.c)) $(notdir $(wildcard $(dir)/*.board.*.c)) $(notdir $(wildcard $(dir)/*.os.*.c)), \
                                                      $(notdir $(wildcard $(dir)/*.c))))
CFILES      +=  $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.arch.$(ATMOSPHERE_ARCH_NAME).c)))
CFILES      +=  $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.board.$(ATMOSPHERE_BOARD_NAME).c)))
CFILES      +=  $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.os.$(ATMOSPHERE_OS_NAME).c)))

CPPFILES    :=	$(foreach dir,$(SOURCES),$(filter-out $(notdir $(wildcard $(dir)/*.arch.*.cpp)) $(notdir $(wildcard $(dir)/*.board.*.cpp)) $(notdir $(wildcard $(dir)/*.os.*.cpp)), \
                                                      $(notdir $(wildcard $(dir)/*.cpp))))
CPPFILES    +=  $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.arch.$(ATMOSPHERE_ARCH_NAME).cpp)))
CPPFILES    +=  $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.board.$(ATMOSPHERE_BOARD_NAME).cpp)))
CPPFILES    +=  $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.os.$(ATMOSPHERE_OS_NAME).cpp)))

SFILES      :=	$(foreach dir,$(SOURCES),$(filter-out $(notdir $(wildcard $(dir)/*.arch.*.s)) $(notdir $(wildcard $(dir)/*.board.*.s)) $(notdir $(wildcard $(dir)/*.os.*.s)), \
                                                      $(notdir $(wildcard $(dir)/*.s))))
SFILES      +=  $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.arch.$(ATMOSPHERE_ARCH_NAME).s)))
SFILES      +=  $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.board.$(ATMOSPHERE_BOARD_NAME).s)))
SFILES      +=  $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.os.$(ATMOSPHERE_OS_NAME).s)))

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
export GCH_FILES    :=	$(foreach hdr,$(PRECOMPILED_HEADERS:.hpp=.gch),$(notdir $(hdr)))
export OFILES 	:=	$(OFILES_BIN) $(OFILES_SRC)
export HFILES_BIN	:=	$(addsuffix .h,$(subst .,_,$(BINFILES)))

export INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
			$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
			-I.

.PHONY: clean all

#---------------------------------------------------------------------------------
all: $(ATMOSPHERE_LIBRARY_DIR)/$(TARGET).a

$(ATMOSPHERE_LIBRARY_DIR):
	@[ -d $@ ] || mkdir -p $@

$(ATMOSPHERE_BUILD_DIR):
	@[ -d $@ ] || mkdir -p $@

$(ATMOSPHERE_LIBRARY_DIR)/$(TARGET).a : $(ATMOSPHERE_LIBRARY_DIR) $(ATMOSPHERE_BUILD_DIR) $(SOURCES) $(INCLUDES)
	@$(MAKE) BUILD=$(ATMOSPHERE_BUILD_DIR) OUTPUT=$(CURDIR)/$@ \
	DEPSDIR=$(CURDIR)/$(ATMOSPHERE_BUILD_DIR) \
	--no-print-directory -C $(ATMOSPHERE_BUILD_DIR) \
	-f $(CURDIR)/arm.mk

dist-bin: all
	@tar --exclude=*~ -cjf $(TARGET).tar.bz2 include $(ATMOSPHERE_LIBRARY_DIR)

dist-src:
	@tar --exclude=*~ -cjf $(TARGET)-src.tar.bz2 include source arm.mk

dist: dist-src dist-bin

#---------------------------------------------------------------------------------
clean:
	@echo clean ...
	@rm -fr $(ATMOSPHERE_BUILD_DIR) $(ATMOSPHERE_LIBRARY_DIR) *.bz2

#---------------------------------------------------------------------------------
else

DEPENDS	:=	$(OFILES:.o=.d) $(GCH_FILES:.gch=.d)

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
$(OUTPUT)	:	$(OFILES)

$(OFILES)	:	$(GCH_FILES)

$(OFILES_SRC)	: $(HFILES_BIN)

libc.o: CFLAGS += -fno-builtin -fno-lto

#---------------------------------------------------------------------------------
%_bin.h %.bin.o	:	%.bin
#---------------------------------------------------------------------------------
	@echo $(notdir $<)
	@$(bin2o)


-include $(DEPENDS)

#---------------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------------

