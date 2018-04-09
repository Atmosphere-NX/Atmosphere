rwildcard = $(foreach d, $(wildcard $1*), $(filter $(subst *, %, $2), $d) $(call rwildcard, $d/, $2))

ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

include $(DEVKITARM)/base_tools

dir_source := src
dir_out := out
dir_exosphere := ../exosphere
dir_loader := fusee-secondary
dir_stage1 := fusee-primary

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

.PHONY: all
all: $(dir_out)

.PHONY: clean
clean:
	@$(MAKE) -C $(dir_exosphere) clean
	@$(MAKE) -C $(dir_loader) clean
	@$(MAKE) -C $(dir_stage1) clean
	@rm -rf $(dir_out)

.PHONY: $(dir_exosphere) $(dir_loader) $(dir_stage1)

$(dir_out): $(dir_loader) $(dir_stage1)
	@mkdir -p "$(dir_out)"
	@$(MAKE) -C $(dir_exosphere)

$(dir_exosphere)/out/exosphere.bin: $(dir_exosphere)
	@$(MAKE) -C $<

$(dir_loader):
	@mkdir -p "$(dir_out)"
	@$(MAKE) -C $(dir_loader)
	@cp "$(dir_loader)/out/$(dir_loader).bin" "out/$(dir_loader).bin"

$(dir_stage1):
	@mkdir -p "$(dir_out)"
	@$(MAKE) -C $(dir_stage1)
	@cp "$(dir_stage1)/out/$(dir_stage1).bin" "out/$(dir_stage1).bin"
    