#---------------------------------------------------------------------------------
# Define the atmosphere board and cpu
#---------------------------------------------------------------------------------
export ATMOSPHERE_BOARD := nx-hac-001
export ATMOSPHERE_CPU   := arm-cortex-a57

#---------------------------------------------------------------------------------
# pull in common atmosphere configuration
#---------------------------------------------------------------------------------
THIS_MAKEFILE     := $(abspath $(lastword $(MAKEFILE_LIST)))
CURRENT_DIRECTORY := $(abspath $(dir $(THIS_MAKEFILE)))
include  $(dir $(abspath $(lastword $(MAKEFILE_LIST))))/../../libraries/config/templates/exosphere.mk

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
BINFILES    :=

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

export OFILES_SRC	:=	$(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)
export OFILES 	:= $(OFILES_SRC) program.lz4.o boot_code.lz4.o

export INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
			$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
			-I.

export LIBPATHS	:=	$(foreach dir,$(LIBDIRS),-L$(dir)/$(ATMOSPHERE_LIBRARY_DIR))

export TOPDIR := $(CURRENT_DIRECTORY)

OUTPUT_BASE := $(TOPDIR)/$(notdir $(TOPDIR))

#---------------------------------------------------------------------------------

ATMOSPHERE_BUILD_CONFIGS :=
all: release

define ATMOSPHERE_ADD_TARGET

ATMOSPHERE_BUILD_CONFIGS += $(strip $1)

$(strip $1): check_libexo_$(strip $1) $$(ATMOSPHERE_BUILD_DIR)/$(strip $1)
	@$$(MAKE) __RECURSIVE__=1 OUTPUT=$$(OUTPUT_BASE)$(strip $2) $(3) \
	ATMOSPHERE_BUILD_TARGET_IDENTIFIER=$(strip $1)\
	ATMOSPHERE_BUILD_TARGET_BINARY_SUFFIX=$(strip $2)\
	DEPSDIR=$$(CURDIR)/$$(ATMOSPHERE_BUILD_DIR)/$(strip $1) \
	LIBEXOSPHERE_NAME=exosphere$(strip $2) \
	--no-print-directory -C $$(ATMOSPHERE_BUILD_DIR)/$(strip $1) \
	-f $$(THIS_MAKEFILE)

check_libexo_$(strip $1):
	@$$(MAKE) --no-print-directory -C $$(ATMOSPHERE_LIBRARIES_DIR)/libexosphere $$(ATMOSPHERE_ARCH_NAME)-$(strip $1)

clean-$(strip $1):
	@echo clean $(strip $1) ...
	@rm -fr $$(ATMOSPHERE_BUILD_DIR)/$(strip $1) $$(OUTPUT_BASE)$(strip $2).bin $$(OUTPUT_BASE)$(strip $2).elf $$(OUTPUT_BASE)$(strip $2).lz4 boot_code$(strip $2).lz4

endef

$(eval $(call ATMOSPHERE_ADD_TARGET, release,     , \
	ATMOSPHERE_BUILD_SETTINGS="-DAMS_FORCE_DISABLE_DETAILED_ASSERTIONS" \
))

$(eval $(call ATMOSPHERE_ADD_TARGET, debug, _debug, \
	ATMOSPHERE_BUILD_SETTINGS="-DAMS_FORCE_DISABLE_DETAILED_ASSERTIONS -DAMS_BUILD_FOR_DEBUGGING" \
))

$(eval $(call ATMOSPHERE_ADD_TARGET, audit, _audit, \
	ATMOSPHERE_BUILD_SETTINGS="-DAMS_FORCE_DISABLE_DETAILED_ASSERTIONS -DAMS_BUILD_FOR_AUDITING" \
))

$(ATMOSPHERE_BUILD_DIR)/%:
	@[ -d $@ ] || mkdir -p $@

#---------------------------------------------------------------------------------
clean: $(foreach config,$(ATMOSPHERE_BUILD_CONFIGS),clean-$(config))

.PHONY: all clean $(foreach config,$(ATMOSPHERE_BUILD_CONFIGS),$(config) clean-$(config))

#---------------------------------------------------------------------------------
else

DEPENDS	:=	$(OFILES:.o=.d)

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------

$(OUTPUT).bin	:	$(OUTPUT).elf
	$(OBJCOPY) -S -O binary --set-section-flags .bss=alloc,load,contents $< $@
	@echo built ... $(notdir $@)

$(OUTPUT).elf	:	$(OFILES)

$(OFILES)	:	$(ATMOSPHERE_LIBRARIES_DIR)/libexosphere/$(ATMOSPHERE_LIBRARY_DIR)/lib$(LIBEXOSPHERE_NAME).a


program.lz4.o: program_lz4.h

program.lz4.o	program_lz4.h:	$(TOPDIR)/../program/program$(ATMOSPHERE_BUILD_TARGET_BINARY_SUFFIX).lz4
	@echo $(notdir $<)
	@rm -rf tmp_program_$(ATMOSPHERE_BUILD_TARGET_IDENTIFIER)
	@mkdir -p tmp_program_$(ATMOSPHERE_BUILD_TARGET_IDENTIFIER)
	@cp $(TOPDIR)/../program/program$(ATMOSPHERE_BUILD_TARGET_BINARY_SUFFIX).lz4 tmp_program_$(ATMOSPHERE_BUILD_TARGET_IDENTIFIER)/program.lz4
	@bin2s -a 8 -H program_lz4.h tmp_program_$(ATMOSPHERE_BUILD_TARGET_IDENTIFIER)/program.lz4 | $(AS) -o program.lz4.o
	@rm -rf tmp_program_$(ATMOSPHERE_BUILD_TARGET_IDENTIFIER)

$(TOPDIR)/../program/program$(ATMOSPHERE_BUILD_TARGET_BINARY_SUFFIX).lz4:
	@$(MAKE) __RECURSIVE__=0 --no-print-directory -C $(TOPDIR)/../program $(ATMOSPHERE_BUILD_TARGET_IDENTIFIER)

boot_code.lz4.o: boot_code_lz4.h

boot_code_lz4.h:	$(TOPDIR)/../program/boot_code$(ATMOSPHERE_BUILD_TARGET_BINARY_SUFFIX).lz4
	@echo $(notdir $<)
	@rm -rf tmp_boot_code_$(ATMOSPHERE_BUILD_TARGET_IDENTIFIER)
	@mkdir -p tmp_boot_code_$(ATMOSPHERE_BUILD_TARGET_IDENTIFIER)
	@cp $(TOPDIR)/../program/boot_code$(ATMOSPHERE_BUILD_TARGET_BINARY_SUFFIX).lz4 tmp_boot_code_$(ATMOSPHERE_BUILD_TARGET_IDENTIFIER)/boot_code.lz4
	@bin2s -a 8 -H boot_code_lz4.h tmp_boot_code_$(ATMOSPHERE_BUILD_TARGET_IDENTIFIER)/boot_code.lz4 | $(AS) -o boot_code.lz4.o
	@rm -rf tmp_boot_code_$(ATMOSPHERE_BUILD_TARGET_IDENTIFIER)

$(TOPDIR)/../program/boot_code$(ATMOSPHERE_BUILD_TARGET_BINARY_SUFFIX).lz4: $(TOPDIR)/../program/program$(ATMOSPHERE_BUILD_TARGET_BINARY_SUFFIX).lz4

%.elf:
	@echo linking $(notdir $@)
	$(LD) $(LDFLAGS) $(OFILES) $(LIBPATHS) $(LIBS) -o $@
	@$(NM) -CSn $@ > $(notdir $*.lst)

$(OFILES_SRC)	: program_lz4.h boot_code_lz4.h


-include $(DEPENDS)

#---------------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------------
