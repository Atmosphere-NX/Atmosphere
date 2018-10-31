#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------

ifeq ($(MESOSPHERE_BOARD),)
export MESOSPHERE_BOARD := nintendo-switch
endif

#---------------------------------------------------------------------------------
# TARGET is the name of the output
# BUILD is the directory where object files & intermediate files will be placed
# SOURCES is a list of directories containing source code
# DATA is a list of directories containing data files
# INCLUDES is a list of directories containing header files
#---------------------------------------------------------------------------------
TARGET		:=	$(notdir $(CURDIR))
BUILD		:=	build

ifneq ($(BUILD),$(notdir $(CURDIR)))
export CONFIG_DIR		:=	$(CURDIR)/config
ifeq ($(MESOSPHERE_BOARD),nintendo-switch)
export BOARD_MAKE_DIR	:=	$(CURDIR)/config/board/nintendo/switch
export ARCH_MAKE_DIR	:=	$(CURDIR)/config/arch/arm64
endif
endif

include $(CONFIG_DIR)/rules.mk
include $(CONFIG_DIR)/common.mk
include $(ARCH_MAKE_DIR)/arch.mk
include $(BOARD_MAKE_DIR)/board.mk

SOURCES		:=	$(COMMON_SOURCES_DIRS) $(ARCH_SOURCE_DIRS) $(BOARD_SOURCE_DIRS)
DATA		:=	data
INCLUDES	:=	include ../common/include

DEFINES		:=	$(COMMON_DEFINES) $(ARCH_DEFINES) $(BOARD_DEFINES)

#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------
SETTING		:=	$(COMMON_SETTING) $(ARCH_SETTING) $(BOARD_SETTING)

CFLAGS		:=	$(SETTING) $(DEFINES) $(COMMON_CFLAGS) $(ARCH_CFLAGS) $(BOARD_CFLAGS)
CFLAGS		+=	$(INCLUDE)

CXXFLAGS	:=	$(CFLAGS) $(COMMON_CXXFLAGS) $(ARCH_CXXFLAGS) $(BOARD_CXXFLAGS)

ASFLAGS		:=	-g $(SETTING)
LDFLAGS		=	-specs=$(ARCH_MAKE_DIR)/linker.specs $(SETTING) $(COMMON_LDFLAGS)

LIBS	:=

#---------------------------------------------------------------------------------
# list of directories containing libraries, this must be the top level containing
# include and lib
#---------------------------------------------------------------------------------
LIBDIRS	:=


#---------------------------------------------------------------------------------
# no real need to edit anything past this point unless you need to add additional
# rules for different file extensions
#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#---------------------------------------------------------------------------------

export OUTPUT	:=	$(CURDIR)/$(TARGET)
export TOPDIR	:=	$(CURDIR)

export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
					$(foreach dir,$(DATA),$(CURDIR)/$(dir))

export DEPSDIR	:=	$(CURDIR)/$(BUILD)

CFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
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
					-I$(CURDIR)/$(BUILD)

export LIBPATHS	:=	$(foreach dir,$(LIBDIRS),-L$(dir)/lib)

.PHONY: $(BUILD) clean all

#---------------------------------------------------------------------------------
all: $(BUILD)

$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

#---------------------------------------------------------------------------------
clean:
	@echo clean ...
	@rm -fr $(BUILD) $(TARGET).bin $(TARGET).elf


#---------------------------------------------------------------------------------
else
.PHONY:	all

DEPENDS	:=	$(OFILES:.o=.d)

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
all	:	$(OUTPUT).bin

$(OUTPUT).bin	:	$(OUTPUT).elf
	$(OBJCOPY) -S -O binary $< $@
	@echo built ... $(notdir $@)

$(OUTPUT).elf	:	$(OFILES)

%.elf:
	@echo linking $(notdir $@)
	$(LD) $(LDFLAGS) $(OFILES) $(LIBPATHS) $(LIBS) -o $@
	@$(NM) -CSn $@ > $(notdir $*.lst)

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
