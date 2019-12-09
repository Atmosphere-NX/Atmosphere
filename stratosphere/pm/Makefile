#---------------------------------------------------------------------------------
# pull in common stratosphere sysmodule configuration
#---------------------------------------------------------------------------------
include $(dir $(abspath $(lastword $(MAKEFILE_LIST))))/../../libraries/config/templates/stratosphere.mk

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

export BUILD_EXEFS_SRC := $(TOPDIR)/$(EXEFS_SRC)

ifeq ($(strip $(CONFIG_JSON)),)
	jsons := $(wildcard *.json)
	ifneq (,$(findstring $(TARGET).json,$(jsons)))
		export APP_JSON := $(TOPDIR)/$(TARGET).json
	else
		ifneq (,$(findstring config.json,$(jsons)))
			export APP_JSON := $(TOPDIR)/config.json
		endif
	endif
else
	export APP_JSON := $(TOPDIR)/$(CONFIG_JSON)
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
	@rm -fr $(BUILD) $(TARGET).kip $(TARGET).elf


#---------------------------------------------------------------------------------
else
.PHONY:	all

DEPENDS	:=	$(OFILES:.o=.d)

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
all	:	$(OUTPUT).kip

$(OUTPUT).kip	:	$(OUTPUT).elf

$(OUTPUT).elf	:	$(OFILES)

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
