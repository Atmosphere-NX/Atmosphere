rwildcard = $(foreach d, $(wildcard $1*), $(filter $(subst *, %, $2), $d) $(call rwildcard, $d/, $2))

ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

include $(DEVKITPRO)/devkitA64/base_tools

name := exosphere

dir_source := src
dir_build := build
dir_out := out
dir_bpmpfw := bpmpfw

ARCH := -march=armv8-a -mtune=cortex-a57

ASFLAGS := -g $(ARCH)

CFLAGS = \
	$(ARCH) \
	-g \
	-O2 \
	-ffunction-sections \
	-fdata-sections \
	-fomit-frame-pointer \
	-std=gnu11 \
	-Werror \
	-Wall

LDFLAGS = -g $(ARCH) -nostartfiles -Wl,--nmagic,--gc-sections

objects = $(patsubst $(dir_source)/%.s, $(dir_build)/%.o, \
          $(patsubst $(dir_source)/%.c, $(dir_build)/%.o, \
          $(call rwildcard, $(dir_source), *.s *.c)))

.PHONY: all
all: $(dir_out)/$(name).bin

.PHONY: clean
clean:
	@$(MAKE) -C $(dir_bpmpfw) clean
	@rm -rf $(dir_build)
	@rm -rf $(dir_out)

.PHONY: $(dir_bpmpfw)

$(dir_out)/$(name).bin: $(dir_build)/$(name).elf
	@mkdir -p "$(@D)"
	$(OBJCOPY) -S -O binary $< $@

$(dir_build)/$(name).elf: $(objects)
	$(LINK.o) -T linker.ld $(OUTPUT_OPTION) $^

$(dir_bpmpfw)/out/bpmpfw.bin: $(dir_bpmpfw)
	@$(MAKE) -C $<

$(dir_build)/bpmpfw.bin: $(dir_bpmpfw)/out/bpmpfw.bin
	@mkdir -p "$(@D)"
	@cp $< $(@D)
 
$(dir_build)/%.bin.o: $(dir_build)/%.bin
	@$(bin2o)

$(dir_build)/%.o: $(dir_source)/%.c $(dir_build)/bpmpfw.bin.o
	@mkdir -p "$(@D)"
	$(COMPILE.c) $(OUTPUT_OPTION) $<

$(dir_build)/%.o: $(dir_source)/%.s
	@mkdir -p "$(@D)"
	$(COMPILE.c) -x assembler-with-cpp $(OUTPUT_OPTION) $<
