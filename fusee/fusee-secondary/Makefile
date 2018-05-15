rwildcard = $(foreach d, $(wildcard $1*), $(filter $(subst *, %, $2), $d) $(call rwildcard, $d/, $2))

ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

include $(DEVKITARM)/base_tools

name := fusee-secondary

dir_source := src
dir_build := build
dir_exosphere := ../../exosphere
dir_thermosphere := ../../thermosphere
dir_stratosphere := ../../stratosphere
dir_out := out

ARCH := -march=armv4t -mtune=arm7tdmi -marm

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
	-Wall \
	-fstrict-volatile-bitfields \
	-DFUSEE_STAGE2_SRC

LDFLAGS = -specs=linker.specs -g $(ARCH)

bundled =	$(dir_exosphere)/out/exosphere.bin $(dir_thermosphere)/out/thermosphere.bin \
			$(dir_stratosphere)/loader/loader.kip $(dir_stratosphere)/pm/pm.kip \
			$(dir_stratosphere)/sm/sm.kip $(dir_stratosphere)/boot/boot_100.kip $(dir_stratosphere)/boot/boot_200.kip

objects =	$(patsubst $(dir_source)/%.s, $(dir_build)/%.o, \
			$(patsubst $(dir_source)/%.c, $(dir_build)/%.o, \
			$(call rwildcard, $(dir_source), *.s *.c))) \
			$(dir_build)/bundled.o

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

$(dir_build)/bundled.o: $(bundled)
	@bin2s $^ | $(AS) -o $(@)

$(dir_build)/%.o: $(dir_source)/%.c
	@mkdir -p "$(@D)"
	$(COMPILE.c) $(OUTPUT_OPTION) $<

$(dir_build)/%.o: $(dir_source)/%.s
	@mkdir -p "$(@D)"
	$(COMPILE.c) -x assembler-with-cpp $(OUTPUT_OPTION) $<
