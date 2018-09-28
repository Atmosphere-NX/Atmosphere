/*
 * Copyright (c) 2018 Atmosphère-NX
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
 
#include <inttypes.h>

#include "exception_handlers.h"
#include "utils.h"
#include "lib/log.h"

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

    print(SCREEN_LOG_LEVEL_ERROR, "\nSomething went wrong...\n");

    code_dump_size = safecpy(code_dump, (const void *)instr_addr, CODE_DUMP_SIZE);
    stack_dump_size = safecpy(stack_dump, (const void *)registers[13], STACK_DUMP_SIZE);

    print(SCREEN_LOG_LEVEL_ERROR | SCREEN_LOG_LEVEL_NO_PREFIX, "\nException type: %s\n",
            exception_names[exception_type]);
    print(SCREEN_LOG_LEVEL_ERROR | SCREEN_LOG_LEVEL_NO_PREFIX, "\nRegisters:\n\n");

    /* Print r0 to pc. */
    for (int i = 0; i < 16; i += 2) {
        print(SCREEN_LOG_LEVEL_ERROR | SCREEN_LOG_LEVEL_NO_PREFIX, "%-7s%08"PRIX32"       %-7s%08"PRIX32"\n",
                register_names[i], registers[i], register_names[i+1], registers[i+1]);
    }

    /* Print cpsr. */
    print(SCREEN_LOG_LEVEL_ERROR | SCREEN_LOG_LEVEL_NO_PREFIX, "%-7s%08"PRIX32"\n", register_names[16], registers[16]);

    print(SCREEN_LOG_LEVEL_ERROR | SCREEN_LOG_LEVEL_NO_PREFIX, "\nCode dump:\n");
    hexdump(code_dump, code_dump_size, instr_addr);
    print(SCREEN_LOG_LEVEL_ERROR | SCREEN_LOG_LEVEL_NO_PREFIX, "\nStack dump:\n");
    hexdump(stack_dump, stack_dump_size, registers[13]);
    print(SCREEN_LOG_LEVEL_ERROR | SCREEN_LOG_LEVEL_NO_PREFIX, "\n");
    fatal_error("An exception occured!\n");
}
