#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------

ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>/devkitpro")
endif

TOPDIR ?= $(CURDIR)
include $(DEVKITPRO)/devkitA64/base_rules

AMSBRANCH := $(shell git symbolic-ref --short HEAD)
AMSHASH = $(shell git rev-parse --short=16 HEAD)
AMSREV := $(AMSBRANCH)-$(shell git rev-parse --short HEAD)

ifneq (, $(strip $(shell git status --porcelain 2>/dev/null)))
    AMSREV := $(AMSREV)-dirty
endif

ifeq ($(PLATFORM), qemu)

export PLATFORM	:= qemu

PLATFORM_SOURCES	:= src/platform/qemu
PLATFORM_DEFINES	:= -DPLATFORM_QEMU

else ifeq ($(PLATFORM), tegra-t210-arm-tf)

export PLATFORM := tegra-t210-arm-tf

PLATFORM_SOURCES	:= src/platform/tegra
PLATFORM_DEFINES	:= -DPLATFORM_TEGRA -DPLATFORM_TEGRA_T210_ARM_TF

else

export PLATFORM	:= tegra-t210-nintendo

PLATFORM_SOURCES	:= src/platform/tegra
PLATFORM_DEFINES	:= -DPLATFORM_TEGRA -D DPLATFORM_TEGRA_T210_NINTENDO

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
SOURCES		:=	src src/platform $(PLATFORM_SOURCES)
DATA		:=	data
INCLUDES	:=	include ../common/include

#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------
ARCH	:=	-march=armv8-a -mtune=cortex-a57 -mgeneral-regs-only -ffixed-x18 #<- important
DEFINES	:=	-D__CCPLEX__ -DATMOSPHERE_GIT_BRANCH=\"$(AMSBRANCH)\" -DATMOSPHERE_GIT_REV=\"$(AMSREV)\"\
			-DATMOSPHERE_RELEASE_VERSION_HASH="0x$(AMSHASH)" $(PLATFORM_DEFINES)
CFLAGS	:= \
	-g \
	-Os \
	-ffunction-sections \
	-fdata-sections \
	-fomit-frame-pointer \
	-fno-asynchronous-unwind-tables \
	-fstrict-volatile-bitfields \
	-fno-unwind-tables \
	-std=gnu11 \
	-Werror \
	-Wall \
	-Wno-main \
	$(ARCH) $(DEFINES)

CFLAGS	+=	$(INCLUDE) 

CXXFLAGS	:= $(CFLAGS) -fno-rtti -fno-exceptions -std=gnu++11

ASFLAGS	:=	-g $(ARCH)
LDFLAGS	=	-specs=$(TOPDIR)/linker.specs -nostartfiles -nostdlib -g $(ARCH) -Wl,-Map,$(notdir $*.map)

LIBS	:=	-lgcc

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

export OFILES_BIN	:=	$(addsuffix .o,$(BINFILES))
export OFILES_SRC	:=	$(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)
export OFILES 		:=	$(OFILES_BIN) $(OFILES_SRC)
export HFILES_BIN	:=	$(addsuffix .h,$(subst .,_,$(BINFILES)))

export INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
			$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
			-I$(CURDIR)/$(BUILD)

export LIBPATHS	:=	$(foreach dir,$(LIBDIRS),-L$(dir)/lib)

.PHONY: $(BUILD) clean all qemu qemudbg

#---------------------------------------------------------------------------------
all: $(BUILD)

ifeq ($(PLATFORM), qemu)

export QEMU := qemu-system-aarch64

QEMUFLAGS :=	-nographic -machine virt,virtualization=on,gic-version=2 -cpu cortex-a57 -smp 4 -m 1024\
			-kernel thermosphere.elf -d unimp,guest_errors -semihosting-config enable,target=native -serial mon:stdio

qemu: all
	@$(QEMU) $(QEMUFLAGS)

qemudbg: all
	@$(QEMU) $(QEMUFLAGS) -s -S

endif

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

$(OFILES_SRC)	: $(HFILES_BIN)

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
