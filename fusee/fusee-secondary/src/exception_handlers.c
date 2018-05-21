#include <inttypes.h>

#include "exception_handlers.h"
#include "lib/driver_utils.h"
#include "utils.h"
#include "display/video_fb.h"

#define CODE_DUMP_SIZE      0x30
#define STACK_DUMP_SIZE     0x60

extern const uint32_t exception_handler_table[];

static const char *exception_names[] = {
    "Reset", "Undefined instruction", "SWI", "Prefetch abort", "Data abort", "Reserved", "IRQ", "FIQ",
};

static const char *register_names[] = {
    "R0", "R1", "R2", "R3", "R4", "R5", "R6", "R7", "R8", "R9", "R10", "R11", "R12",
    "SP", "LR", "PC", "CPSR",
};

void setup_exception_handlers(void) {
    volatile uint32_t *bpmp_exception_handler_table = (volatile uint32_t *)0x6000F200;
    for (int i = 0; i < 8; i++) {
        if (exception_handler_table[i] != 0) {
            bpmp_exception_handler_table[i] = exception_handler_table[i];
        }
    } 
}

void exception_handler_main(uint32_t *registers, unsigned int exception_type) {
    uint8_t code_dump[CODE_DUMP_SIZE];
    uint8_t stack_dump[STACK_DUMP_SIZE];
    size_t  code_dump_size;
    size_t  stack_dump_size;

    uint32_t pc = registers[15];
    uint32_t cpsr = registers[16];

    uint32_t instr_addr = pc + ((cpsr & 0x20) ? 2 : 4) - CODE_DUMP_SIZE;

    printk("\nSomething went wrong...\n");

    code_dump_size = safecpy(code_dump, (const void *)instr_addr, CODE_DUMP_SIZE);
    stack_dump_size = safecpy(stack_dump, (const void *)registers[13], STACK_DUMP_SIZE);

    printk("\nException type: %s\n", exception_names[exception_type]);
    printk("\nRegisters:\n\n");

    /* Print r0 to pc. */
    for (int i = 0; i < 16; i += 2) {
        printk("%-7s%08"PRIX32"       %-7s%08"PRIX32"\n", register_names[i], registers[i], register_names[i+1], registers[i+1]);
    }

    /* Print cpsr. */
    printk("%-7s%08"PRIX32"\n", register_names[16], registers[16]);

    printk("\nCode dump:\n");
    hexdump(code_dump, code_dump_size, instr_addr);
    printk("\nStack dump:\n");
    hexdump(stack_dump, stack_dump_size, registers[13]);
    printk("\n");
    fatal_error("An exception occured!\n");
}
