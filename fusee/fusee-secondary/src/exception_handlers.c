/*
 * Copyright (c) 2018-2020 Atmosphère-NX
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
 
#include <stdio.h>
#include <inttypes.h>

#include "exception_handlers.h"
#include "utils.h"
#include "../../../fusee/common/display/video_fb.h"
#include "../../../fusee/common/log.h"

#define CODE_DUMP_SIZE      0x30
#define STACK_DUMP_SIZE     0x30

extern const uint32_t exception_handler_table[];

static const char *exception_names[] = {
    "Reset", "Undefined instruction", "SWI", "Prefetch abort", "Data abort", "Reserved", "IRQ", "FIQ",
};

static const char *register_names[] = {
    "R0", "R1", "R2", "R3", "R4", "R5", "R6", "R7", "R8", "R9", "R10", "R11", "R12",
    "SP", "LR", "PC", "CPSR",
};

/* Adapted from https://gist.github.com/ccbrown/9722406 */
static void hexdump(const void* data, size_t size, uintptr_t addrbase, char* strbuf) {
    const uint8_t *d = (const uint8_t *)data;
    char ascii[17] = {0};
    ascii[16] = '\0';

    for (size_t i = 0; i < size; i++) {
        if (i % 16 == 0) {
            strbuf += sprintf(strbuf, "%0*" PRIXPTR ": | ", 2 * sizeof(addrbase), addrbase + i);
        }
        strbuf += sprintf(strbuf, "%02X ", d[i]);
        if (d[i] >= ' ' && d[i] <= '~') {
            ascii[i % 16] = d[i];
        } else {
            ascii[i % 16] = '.';
        }
        if ((i+1) % 8 == 0 || i+1 == size) {
            strbuf += sprintf(strbuf, " ");
            if ((i+1) % 16 == 0) {
                strbuf += sprintf(strbuf, "|  %s \n", ascii);
            } else if (i+1 == size) {
                ascii[(i+1) % 16] = '\0';
                if ((i+1) % 16 <= 8) {
                    strbuf += sprintf(strbuf, " ");
                }
                for (size_t j = (i+1) % 16; j < 16; j++) {
                    strbuf += sprintf(strbuf, "   ");
                }
                strbuf += sprintf(strbuf, "|  %s \n", ascii);
            }
        }
    }
}

void setup_exception_handlers(void) {
    volatile uint32_t *bpmp_exception_handler_table = (volatile uint32_t *)0x6000F200;
    for (int i = 0; i < 8; i++) {
        if (exception_handler_table[i] != 0) {
            bpmp_exception_handler_table[i] = exception_handler_table[i];
        }
    } 
}

void exception_handler_main(uint32_t *registers, unsigned int exception_type) {
    char exception_log[0x400] = {0};
    uint8_t code_dump[CODE_DUMP_SIZE] = {0};
    uint8_t stack_dump[STACK_DUMP_SIZE] = {0};
    size_t code_dump_size = 0;
    size_t stack_dump_size = 0;

    uint32_t pc = registers[15];
    uint32_t cpsr = registers[16];
    uint32_t instr_addr = pc + ((cpsr & 0x20) ? 2 : 4) - CODE_DUMP_SIZE;

    sprintf(exception_log, "An exception occurred!\n");
    
    code_dump_size = safecpy(code_dump, (const void *)instr_addr, CODE_DUMP_SIZE);
    stack_dump_size = safecpy(stack_dump, (const void *)registers[13], STACK_DUMP_SIZE);

    sprintf(exception_log + strlen(exception_log), "\nException type: %s\n", exception_names[exception_type]);
    sprintf(exception_log + strlen(exception_log), "\nRegisters:\n");

    /* Print r0 to pc. */
    for (int i = 0; i < 16; i += 2) {
        sprintf(exception_log + strlen(exception_log), "%-7s%08"PRIX32"       %-7s%08"PRIX32"\n",
                register_names[i], registers[i], register_names[i+1], registers[i+1]);
    }

    /* Print cpsr. */
    sprintf(exception_log + strlen(exception_log), "%-7s%08"PRIX32"\n", register_names[16], registers[16]);
    
    /* Print code and stack regions. */
    sprintf(exception_log + strlen(exception_log), "\nCode dump:\n");    
    hexdump(code_dump, code_dump_size, instr_addr, exception_log + strlen(exception_log));
    sprintf(exception_log + strlen(exception_log), "\nStack dump:\n");
    hexdump(stack_dump, stack_dump_size, registers[13], exception_log + strlen(exception_log));
    sprintf(exception_log + strlen(exception_log), "\n");
    
    /* Throw fatal error with the full exception log. */
    fatal_error(exception_log);
}
