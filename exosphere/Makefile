TARGETS := exosphere.bin warmboot.bin program.lz4
CLEAN_TARGETS := exosphere-clean program-clean boot_code-clean warmboot-clean

SUBFOLDERS := $(MODULES)

all: exosphere.bin warmboot.bin

clean: $(CLEAN_TARGETS)
	@rm -f exosphere.bin

exosphere.bin: program.lz4 boot_code.lz4
	$(MAKE) -C loader_stub
	@cp loader_stub/loader_stub.bin exosphere.bin
	@printf LENY >> exosphere.bin
	@echo "Built exosphere.bin..."

program.lz4: build_program
	@cp program/program.lz4 program.lz4
	@cp program/boot_code.lz4 boot_code.lz4

build_program:
	$(MAKE) -C program

warmboot.bin: build_warmboot
	@cp warmboot/warmboot.bin warmboot.bin

build_warmboot:
	$(MAKE) -C warmboot

boot_code.lz4: program.lz4

exosphere-clean:
	$(MAKE) -C loader_stub clean
	@rm -f exosphere.bin

program-clean:
	$(MAKE) -C program clean
	@rm -f program.lz4

warmboot-clean:
	$(MAKE) -C warmboot clean
	@rm -f warmboot.bin

boot_code-clean:
	@rm -f boot_code.lz4

.PHONY: all clean build_program $(CLEAN_TARGETS)
