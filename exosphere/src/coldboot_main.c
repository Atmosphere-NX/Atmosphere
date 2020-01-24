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
 
#include <string.h>
#include "utils.h"
#include "mmu.h"
#include "memory_map.h"
#include "arm.h"
#include "cpu_context.h"

extern uint8_t __pk2ldr_start__[], __pk2ldr_end__[];

void coldboot_main(void) {
    uintptr_t *mmu_l3_table = (uintptr_t *)TZRAM_GET_SEGMENT_ADDRESS(TZRAM_SEGMENT_ID_L3_TRANSLATION_TABLE);
    uintptr_t pk2ldr = TZRAM_GET_SEGMENT_ADDRESS(TZRAM_SEGMENT_ID_PK2LDR);

    /* Clear and unmap pk2ldr (which is reused as exception entry stacks) */
    memset((void *)pk2ldr, 0, __pk2ldr_end__ - __pk2ldr_start__);
    mmu_unmap_range(3, mmu_l3_table, pk2ldr, __pk2ldr_end__ - __pk2ldr_start__);
    tlb_invalidate_all_inner_shareable();

    core_jump_to_lower_el();
}
