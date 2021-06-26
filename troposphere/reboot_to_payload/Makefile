#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------

ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>/devkitpro")
endif

TOPDIR ?= $(CURDIR)
include $(DEVKITPRO)/libnx/switch_rules

#---------------------------------------------------------------------------------
# TARGET is the name of the output
# BUILD is the directory where object files & intermediate files will be placed
# SOURCES is a list of directories containing source code
# DATA is a list of directories containing data files
# INCLUDES is a list of directories containing header files
# EXEFS_SRC is the optional input directory containing data copied into exefs, if anything this normally should only contain "main.npdm".
# ROMFS is the directory containing data to be added to RomFS, relative to the Makefile (Optional)
#
# NO_ICON: if set to anything, do not use icon.
# NO_NACP: if set to anything, no .nacp file is generated.
# APP_TITLE is the name of the app stored in the .nacp file (Optional)
# APP_AUTHOR is the author of the app stored in the .nacp file (Optional)
# APP_VERSION is the version of the app stored in the .nacp file (Optional)
# APP_TITLEID is the titleID of the app stored in the .nacp file (Optional)
# ICON is the filename of the icon (.jpg), relative to the project folder.
#   If not set, it attempts to use one of the following (in this order):
#     - <Project name>.jpg
#     - icon.jpg
#     - <libnx folder>/default_icon.jpg
#---------------------------------------------------------------------------------
TARGET		:=	$(notdir $(CURDIR))
BUILD		:=	build
SOURCES		:=	source
DATA		:=	data
INCLUDES	:=	include
EXEFS_SRC	:=	exefs_src
APP_TITLE	:=	Reboot to Payload
APP_AUTHOR	:=	Atmosphere-NX
APP_VERSION	:=	1.0.0
#ROMFS	:=	romfs

#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------
ARCH	:=	-march=armv8-a -mtune=cortex-a57 -mtp=soft -fPIE

CFLAGS	:=	-g -gdwarf-4 -Wall -O2 -ffunction-sections \
			$(ARCH) $(DEFINES)

CFLAGS	+=	$(INCLUDE) -D__SWITCH__

CXXFLAGS	:= $(CFLAGS) -fno-rtti -fno-exceptions

ASFLAGS	:=	-g -gdwarf-4 $(ARCH)
LDFLAGS	=	-specs=$(DEVKITPRO)/libnx/switch.specs -g -gdwarf-4 $(ARCH) -Wl,-Map,$(notdir $*.map)

LIBS	:= -lnx

#---------------------------------------------------------------------------------
# list of directories containing libraries, this must be the top level containing
# include and lib
#---------------------------------------------------------------------------------
LIBDIRS	:= $(PORTLIBS) $(LIBNX)


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

export OFILES_BIN	:=	$(addsuffix .o,$(BINFILES))
export OFILES_SRC	:=	$(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)
export OFILES 	:=	$(OFILES_BIN) $(OFILES_SRC)
export HFILES_BIN	:=	$(addsuffix .h,$(subst .,_,$(BINFILES)))

export INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
			$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
			-I$(CURDIR)/$(BUILD)

export LIBPATHS	:=	$(foreach dir,$(LIBDIRS),-L$(dir)/lib)

export BUILD_EXEFS_SRC := $(TOPDIR)/$(EXEFS_SRC)

ifeq ($(strip $(ICON)),)
	icons := $(wildcard *.jpg)
	ifneq (,$(findstring $(TARGET).jpg,$(icons)))
		export APP_ICON := $(TOPDIR)/$(TARGET).jpg
	else
		ifneq (,$(findstring icon.jpg,$(icons)))
			export APP_ICON := $(TOPDIR)/icon.jpg
		endif
	endif
else
	export APP_ICON := $(TOPDIR)/$(ICON)
endif

ifeq ($(strip $(NO_ICON)),)
	export NROFLAGS += --icon=$(APP_ICON)
endif

ifeq ($(strip $(NO_NACP)),)
	export NROFLAGS += --nacp=$(CURDIR)/$(TARGET).nacp
endif

ifneq ($(APP_TITLEID),)
	export NACPFLAGS += --titleid=$(APP_TITLEID)
endif

ifneq ($(ROMFS),)
	export NROFLAGS += --romfsdir=$(CURDIR)/$(ROMFS)
endif

.PHONY: $(BUILD) clean all

#---------------------------------------------------------------------------------
all: $(BUILD)

$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

#---------------------------------------------------------------------------------
clean:
	@echo clean ...
	@rm -fr $(BUILD) $(TARGET).pfs0 $(TARGET).nso $(TARGET).nro $(TARGET).nacp $(TARGET).elf


#---------------------------------------------------------------------------------
else
.PHONY:	all

DEPENDS	:=	$(OFILES:.o=.d)

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
all	:	$(OUTPUT).nro

ifeq ($(strip $(NO_NACP)),)
$(OUTPUT).nro	:	$(OUTPUT).elf $(OUTPUT).nacp
else
$(OUTPUT).nro	:	$(OUTPUT).elf
endif

$(OUTPUT).elf	:	$(OFILES)

$(OFILES_SRC)	: $(HFILES_BIN)

#---------------------------------------------------------------------------------
# you need a rule like this for each extension you use as binary data
#---------------------------------------------------------------------------------
%.bin.o	%_bin.h :	%.bin
#---------------------------------------------------------------------------------
	@echo $(notdir $<)
	@$(bin2o)

-include $(DEPENDS)

#---------------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------------
