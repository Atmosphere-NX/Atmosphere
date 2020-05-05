TARGETS := exosphere.bin program.lz4
CLEAN_TARGETS := exosphere-clean program-clean boot_code-clean

SUBFOLDERS := $(MODULES)

all: exosphere.bin

clean: $(CLEAN_TARGETS)
	@rm -f exosphere.bin

exosphere.bin: program.lz4 boot_code.lz4
	$(MAKE) -C loader_stub
	@cp loader_stub/loader_stub.bin exosphere.bin
	@echo "Built exosphere.bin..."

program.lz4: check_libexo
	$(MAKE) -C program
	@cp program/program.lz4 program.lz4
	@cp program/boot_code.lz4 boot_code.lz4

boot_code.lz4: program.lz4

check_libexo:
	@$(MAKE) --no-print-directory -C ../libraries/libexosphere

exosphere-clean:
	$(MAKE) -C loader_stub clean
	@rm -f exosphere.bin

program-clean:
	$(MAKE) -C program clean
	@rm -f program.lz4

boot_code-clean:
	$(MAKE) -C boot_code clean
	@rm -f boot_code.lz4

.PHONY: all clean $(CLEAN_TARGETS)
