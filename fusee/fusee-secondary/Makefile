#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------

ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

TOPDIR	?= $(CURDIR)

AMS		:= $(TOPDIR)/../../

include $(DEVKITARM)/base_rules

AMSBRANCH := $(shell git symbolic-ref --short HEAD)
AMSHASH := $(shell git rev-parse --short HEAD)
AMSREV := $(AMSBRANCH)-$(AMSHASH)

ifneq (, $(strip $(shell git status --porcelain 2>/dev/null)))
    AMSREV := $(AMSREV)-dirty
endif

define _bin2o
	bin2s $< | $(AS) -o $(@)
	echo "extern const u8" `(echo $(<F) | sed -e 's/^\([0-9]\)/_\1/' | tr . _ | tr - _)`"_end[];" > `(echo $(<F) | tr . _ | tr - _)`.h
	echo "extern const u8" `(echo $(<F) | sed -e 's/^\([0-9]\)/_\1/' | tr . _ | tr - _)`"[];" >> `(echo $(<F) | tr . _ | tr - _)`.h
	echo "extern const u32" `(echo $(<F) | sed -e 's/^\([0-9]\)/_\1/' | tr . _ | tr - _)`_size";" >> `(echo $(<F) | tr . _ | tr - _)`.h
endef

#---------------------------------------------------------------------------------
# TARGET is the name of the output
# BUILD is the directory where object files & intermediate files will be placed
# SOURCES is a list of directories containing source code
# DATA is a list of directories containing data files
# INCLUDES is a list of directories containing header files
#---------------------------------------------------------------------------------
TARGET		:=	$(notdir $(CURDIR))
BUILD		:=	build
SOURCES		:=	src ../../fusee/common ../../fusee/common/display ../../fusee/common/fatfs ../../fusee/common/sdmmc
DATA		:=	data
INCLUDES	:=	include ../../libraries/libvapours/include

#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------
ARCH	:=	-march=armv4t -mtune=arm7tdmi -marm
DEFINES :=	-D__BPMP__ -DFUSEE_STAGE2_SRC -DATMOSPHERE_GIT_BRANCH=\"$(AMSBRANCH)\" -DATMOSPHERE_GIT_REV=\"$(AMSREV)\" -DATMOSPHERE_GIT_HASH=$(AMSHASH)

CFLAGS	:= \
	-g \
	-O2 \
	-fomit-frame-pointer \
	-ffunction-sections \
	-fdata-sections \
	-std=gnu11 \
	-Werror \
	-Wall \
	-fstrict-volatile-bitfields \
	$(ARCH) $(DEFINES)

CFLAGS	+=	$(INCLUDE)

CXXFLAGS	:= $(CFLAGS) -fno-rtti -fno-exceptions -std=gnu++11

ASFLAGS	:=	-g $(ARCH) $(INCLUDE) $(DEFINES)
LDFLAGS	=	-specs=$(TOPDIR)/linker.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map)

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

export KIPDIRS	:= $(AMS)/stratosphere/loader $(AMS)/stratosphere/ncm $(AMS)/stratosphere/pm $(AMS)/stratosphere/sm $(AMS)/stratosphere/boot $(AMS)/stratosphere/spl $(AMS)/stratosphere/ams_mitm
export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
			$(foreach dir,$(DATA),$(CURDIR)/$(dir)) \
			$(AMS)/exosphere $(AMS)/exosphere/warmboot $(AMS)/exosphere/program/rebootstub \
			$(AMS)/thermosphere $(AMS)/fusee/fusee-primary $(AMS)/sept/sept-primary \
			$(AMS)/sept/sept-secondary $(AMS)/emummc $(AMS)/mesosphere $(AMS)/mesosphere/kernel_ldr $(KIPDIRS)

export DEPSDIR	:=	$(CURDIR)/$(BUILD)

CFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
KIPFILES 	:=	loader.kip ncm.kip pm.kip sm.kip ams_mitm.kip spl.kip boot.kip
BINFILES	:=	$(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*))) fusee-primary.bin \
				exosphere.bin warmboot.bin rebootstub.bin thermosphere.bin splash_screen.bin \
				sept-primary.bin sept-secondary_00.enc sept-secondary_01.enc emummc.kip \
				sept-secondary_dev_00.enc sept-secondary_dev_01.enc mesosphere.bin kernel_ldr.bin \
                mariko_fatal.bin $(KIPFILES)

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

export OFILES_BIN	:=	$(addsuffix .o,$(subst -,_,$(BINFILES)))
export OFILES_SRC	:=	$(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)
export OFILES 		:=	$(OFILES_BIN) $(OFILES_SRC)
export HFILES_BIN	:=	$(addsuffix .h,$(subst .,_,$(subst -,_,$(BINFILES))))

export INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
			$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
			-I$(CURDIR)/$(BUILD)

export LIBPATHS	:=	$(foreach dir,$(LIBDIRS),-L$(dir)/lib)

.PHONY: $(BUILD) clean all
.PHONY: check_fusee_primary check_exosphere check_sept check_emummc check_thermosphere check_stratosphere check_libraries

#---------------------------------------------------------------------------------
all: $(BUILD)

check_fusee_primary:
	@$(MAKE) -C $(AMS)/fusee/fusee-primary all

check_exosphere:
	@$(MAKE) -C $(AMS)/exosphere all

check_sept:
	@$(MAKE) -C $(AMS)/sept all

check_emummc:
	@$(MAKE) -C $(AMS)/emummc EMUMMCDIR=$(AMS)/emummc all

check_thermosphere:
	@$(MAKE) -C $(AMS)/thermosphere all

check_stratosphere: check_libraries
	@$(MAKE) -C $(AMS)/stratosphere all

check_mesosphere: check_libraries
	@$(MAKE) -C $(AMS)/mesosphere all

check_libraries:
	@$(MAKE) -C $(AMS)/libraries all


$(BUILD): check_fusee_primary check_exosphere check_sept check_emummc check_thermosphere check_libraries check_stratosphere check_mesosphere
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

#---------------------------------------------------------------------------------
clean:
	@echo clean ...
	@$(MAKE) -C $(AMS)/fusee/fusee-primary clean
	@$(MAKE) -C $(AMS)/exosphere clean
	@$(MAKE) -C $(AMS)/thermosphere clean
	@$(MAKE) -C $(AMS)/libraries clean
	@$(MAKE) -C $(AMS)/mesosphere clean
	@$(MAKE) -C $(AMS)/stratosphere clean
	@$(MAKE) -C $(AMS)/sept clean
	@$(MAKE) -C $(AMS)/emummc clean
	@rm -fr $(BUILD) $(TARGET).bin $(TARGET).elf

#---------------------------------------------------------------------------------
else
.PHONY:	all

DEPENDS	:=	$(OFILES:.o=.d)

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
all	:	$(OUTPUT).bin

$(OUTPUT).bin	:	$(OUTPUT)-experimental.bin
	@python $(TOPDIR)/fusee_make_standard.py $(OUTPUT)-experimental.bin $(OUTPUT).bin
	@echo built ... $(notdir $@)

$(OUTPUT)-experimental.bin	:	$(OUTPUT)-experimental.elf
	$(OBJCOPY) -S -O binary $< $@
	@echo built ... $(notdir $@)

$(OUTPUT)-experimental.elf	:	$(OFILES)

%.elf: $(OFILES)
	@echo linking $(notdir $@)
	@$(LD) $(LDFLAGS) $(OFILES) $(LIBPATHS) $(LIBS) -o $@
	@$(NM) -CSn $@ > $(notdir $*.lst)

$(OFILES_SRC)	: $(HFILES_BIN)

#---------------------------------------------------------------------------------
# you need a rule like this for each extension you use as binary data
#---------------------------------------------------------------------------------
fusee_primary.bin.o	fusee_primary_bin.h:	fusee-primary.bin
#---------------------------------------------------------------------------------
	@echo $(notdir $<)
	@$(_bin2o)

sept_primary.bin.o	sept_primary_bin.h:	sept-primary.bin
#---------------------------------------------------------------------------------
	@echo $(notdir $<)
	@$(_bin2o)

sept_secondary_00.enc.o	sept_secondary_00_enc.h:	sept-secondary_00.enc
#---------------------------------------------------------------------------------
	@echo $(notdir $<)
	@$(_bin2o)

sept_secondary_01.enc.o	sept_secondary_01_enc.h:	sept-secondary_01.enc
#---------------------------------------------------------------------------------
	@echo $(notdir $<)
	@$(_bin2o)

sept_secondary_dev_00.enc.o	sept_secondary_dev_00_enc.h:	sept-secondary_dev_00.enc
#---------------------------------------------------------------------------------
	@echo $(notdir $<)
	@$(_bin2o)

sept_secondary_dev_01.enc.o	sept_secondary_dev_01_enc.h:	sept-secondary_dev_01.enc
#---------------------------------------------------------------------------------
	@echo $(notdir $<)
	@$(_bin2o)

%.bin.o	%_bin.h:	%.bin
#---------------------------------------------------------------------------------
	@echo $(notdir $<)
	@$(bin2o)

%.bmp.o	%_bmp.h:	%.bmp
#---------------------------------------------------------------------------------
	@echo $(notdir $<)
	@$(bin2o)

%.kip.o	%_kip.h:	%.kip
#---------------------------------------------------------------------------------
	@echo $(notdir $<)
	@$(bin2o)

-include $(DEPENDS)

#---------------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------------
