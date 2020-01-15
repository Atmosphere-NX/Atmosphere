/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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
#include "spinlock.h"
#include "caches.h"

__attribute__((noinline)) bool overlaps(u64 as, u64 ae, u64 bs, u64 be)
{
    if(as <= bs && bs < ae)
        return true;
    if(bs <= as && as < be)
        return true;
    return false;
}

// TODO: put that elsewhere
bool readEl1Memory(void *dst, uintptr_t addr, size_t size)
{
    // Note: what if we read uncached regions/not shared?
    bool valid;

    u64 flags = maskIrq();
    uintptr_t pa = get_physical_address_el1_stage12(&valid, addr);
    restoreInterruptFlags(flags);

    if (!valid) {
        return false;
    }

    memcpy(dst, (const void *)pa, size);

    return true;
}

bool writeEl1Memory(uintptr_t addr, const void *src, size_t size)
{
    bool valid;

    u64 flags = maskIrq();
    uintptr_t pa = get_physical_address_el1_stage12(&valid, addr);
    restoreInterruptFlags(flags);

    if (!valid) {
        return false;
    }

    memcpy((void *)pa, src, size);
    cacheHandleSelfModifyingCodePoU((const void *)pa, size);

    __tlb_invalidate_el1_stage12(); //FIXME FIXME FIXME
    __dsb_sy();
    __isb();

    return true;
}
