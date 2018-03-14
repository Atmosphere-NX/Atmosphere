rwildcard = $(foreach d, $(wildcard $1*), $(filter $(subst *, %, $2), $d) $(call rwildcard, $d/, $2))

ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

include $(DEVKITARM)/base_tools

name := fusee

dir_source := src
dir_build := build
dir_out := out
dir_exosphere := ../exosphere

ARCH := -march=armv4t -mtune=arm7tdmi -mthumb -mthumb-interwork

ASFLAGS := -g $(ARCH)

# For debug builds, replace -O2 by -Og and comment -fomit-frame-pointer out
CFLAGS = \
	$(ARCH) \
	-g \
	-O2 \
	-fomit-frame-pointer \
	-ffunction-sections \
	-fdata-sections \
	-std=gnu11 \
	-Werror \
	-Wall

LDFLAGS = -specs=linker.specs -g $(ARCH)

objects =	$(patsubst $(dir_source)/%.s, $(dir_build)/%.o, \
			$(patsubst $(dir_source)/%.c, $(dir_build)/%.o, \
			$(call rwildcard, $(dir_source), *.s *.c))) \
			$(dir_build)/exosphere.bin.o

define bin2o
	bin2s $< | $(AS) -o $(@)
endef

.PHONY: all
all: $(dir_out)/$(name).bin

.PHONY: clean
clean:
	@$(MAKE) -C $(dir_exosphere) clean
	@rm -rf $(dir_build)
	@rm -rf $(dir_out)

.PHONY: $(dir_exosphere)

$(dir_out)/$(name).bin: $(dir_build)/$(name).elf
	@mkdir -p "$(@D)"
	$(OBJCOPY) -S -O binary $< $@

$(dir_build)/$(name).elf: $(objects)
	$(LINK.o) $(OUTPUT_OPTION) $^

$(dir_exosphere)/out/exosphere.bin: $(dir_exosphere)
	@$(MAKE) -C $<

$(dir_build)/exosphere.bin: $(dir_exosphere)/out/exosphere.bin
	@mkdir -p "$(@D)"
	@cp $< $(@D)

$(dir_build)/%.bin.o: $(dir_build)/%.bin
	@$(bin2o)

$(dir_build)/%.o: $(dir_source)/%.c
	@mkdir -p "$(@D)"
	$(COMPILE.c) $(OUTPUT_OPTION) $<

$(dir_build)/%.o: $(dir_source)/%.s
	@mkdir -p "$(@D)"
	$(COMPILE.c) -x assembler-with-cpp $(OUTPUT_OPTION) $<
