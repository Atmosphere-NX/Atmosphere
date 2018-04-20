rwildcard = $(foreach d, $(wildcard $1*), $(filter $(subst *, %, $2), $d) $(call rwildcard, $d/, $2))

ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>devkitPro")
endif

include $(DEVKITPRO)/devkitA64/base_tools

name := thermosphere

dir_source := src
dir_build := build
dir_out := out

ARCH := -march=armv8-a -mtune=cortex-a57

ASFLAGS := -g $(ARCH)

CFLAGS = \
	$(ARCH) \
	-g \
	-O2 \
	-ffunction-sections \
	-fdata-sections \
	-mgeneral-regs-only \
	-fomit-frame-pointer \
	-std=gnu11 \
	-Werror \
	-Wall \
	-Wno-main

LDFLAGS = -specs=linker.specs -g $(ARCH)

objects =	$(patsubst $(dir_source)/%.s, $(dir_build)/%.o, \
			$(patsubst $(dir_source)/%.c, $(dir_build)/%.o, \
			$(patsubst $(dir_source)/%.S, $(dir_build)/%.o, \
			$(call rwildcard, $(dir_source), *.s *.c *.S))))


define bin2o
	bin2s $< | $(AS) -o $(@)
endef

.PHONY: all
all: $(dir_out)/$(name).bin

.PHONY: clean
clean:
	@rm -rf $(dir_build)
	@rm -rf $(dir_out)

$(dir_out)/$(name).bin: $(dir_build)/$(name).elf
	@mkdir -p "$(@D)"
	$(OBJCOPY) -S -O binary $< $@

$(dir_build)/$(name).elf: $(objects)
	$(LINK.o) $(OUTPUT_OPTION) $^

$(dir_build)/%.bin.o: $(dir_build)/%.bin
	@$(bin2o)

$(dir_build)/%.o: $(dir_source)/%.c
	@mkdir -p "$(@D)"
	$(COMPILE.c) $(OUTPUT_OPTION) $<

$(dir_build)/%.o: $(dir_source)/%.s
	@mkdir -p "$(@D)"
	$(COMPILE.c) -x assembler-with-cpp $(OUTPUT_OPTION) $<

