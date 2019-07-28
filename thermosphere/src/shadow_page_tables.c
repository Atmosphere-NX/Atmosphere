/*
 * Copyright (c) 2019 Atmosphère-NX
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


#include "shadow_page_tables.h"

#ifdef A32_SUPPORTED
static void replacePageTableShortL2(u32 *ttbl)
{
    u32 inc;
    for (u32 i = 0; i < BIT(8); i += inc) {
        u32 type = ttbl[i] & 3;
        switch (type) {
            case 0:
                // Fault
                inc = 1;
                break;
            case 1:
                // Large page
                // Nothing to replace at this granularity level
                inc = 16;
                break;
            case 2:
            case 3:
                // Section or supersection
                // Nothing to replace at this granularity level yet
                // TODO
                inc = 1;
                break;
        }
    }
}

void replacePageTableShort(u32 *ttbl, u32 n)
{
    //u32 mask = MASK2(31 - n, 20);
    u32 inc;
    for (u32 i = 0; i < BIT(12 - n); i += inc) {
        u32 type = ttbl[i] & 3;
        switch (type) {
            case 0:
                // Fault
                inc = 1;
                break;
            case 1:
                // L2 tbl
                replacePageTableShortL2((u32 *)(uintptr_t)(ttbl[i] & ~MASK(10)));
                inc = 1;
                break;
            case 2:
            case 3:
                // Section or supersection
                // Nothing to replace at this granularity level yet
                inc = (ttbl[i] & BIT(18)) ? 16 : 0;
                break;
        }
    }
}
#endif

static void replacePageTableLongImpl(u64 *ttbl, u32 level, u32 nbits)
{
    for (u32 i = 0; i < BIT(nbits); i++) {
        u64 type = ttbl[i] & 3;
        switch (type) {
            case 0:
            case 2:
                // Fault
                break;
            case 1:
                // Block (L1 or L2) or invalid (L0 or L3)
                // Nothing to do at this granularity level anyway.
                break;
            case 3: {
                // Lower-level table or page
                if (level < 3) {
                    uintptr_t addr = ttbl[i] & MASK2L(47, 12);
                    replacePageTableLongImpl((u64 *)addr, level + 1, 9);
                } else {
                    u64 pa = ttbl[i] & MASK2L(47, 12);
                    // FIXME
                    if (pa == 0x50042000ull) {
                        ttbl[i] = (ttbl[i] & ~MASK2L(47, 12)) | 0x50046000ull;
                    }
                }

                break;
            }

            default:
                break;
        }
    }
}

void replacePageTableLong(u64 *ttbl, u32 txsz)
{
    u32 startBit = 63 - txsz;

    // Initial level 3 for 4KB granule: "c. Only available if ARMv8.4-TTST is implemented, while the PE is executing in AArch64 state." (Arm Arm).
    // This means there is a maximum value for TxSz...

    if (startBit >= 48) {
        // Invalid
        return;
    } else if (startBit >= 39) {
        replacePageTableLongImpl(ttbl, 0, startBit - 38);
    } else if (startBit >= 30) {
        replacePageTableLongImpl(ttbl, 1, startBit - 29);
    } else if (startBit >= 21) {
        replacePageTableLongImpl(ttbl, 2, startBit - 20);
    } else if (startBit >= 12) {
        replacePageTableLongImpl(ttbl, 3, startBit - 11);
    }
}