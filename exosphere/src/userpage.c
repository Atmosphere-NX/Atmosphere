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
 
#include <string.h>

#include "utils.h"
#include "userpage.h"
#include "memory_map.h"
#include "arm.h"

static uintptr_t g_user_page_user_address = 0ull;

static inline uintptr_t get_page_for_address(void *address) {
    return ((uintptr_t)(address)) & ~0xFFFULL;
}

/* Create a user page reference for the desired address. */
/* Returns 1 on success, 0 on failure. */
bool upage_init(upage_ref_t *upage, void *user_address) {
    upage->user_address = get_page_for_address(user_address);
    upage->secure_monitor_address = 0ull;

    if (g_user_page_user_address != 0ull) {
        /* Different physical address indicate SPL was rebooted, or another process got access to svcCallSecureMonitor. Panic. */
        if (g_user_page_user_address != upage->user_address) {
            generic_panic();
        }
        upage->secure_monitor_address = USER_PAGE_SECURE_MONITOR_ADDR;
    } else {
        /* Validate SPL's physically random address (must be in DRAM (supports up to 6GB, retail console have 4GB) and page-aligned). */
        if ((upage->user_address - 0x80000000ull) < (6ull << 30) && ((uintptr_t)user_address & 0xFFF) == 0) {
            static const uint64_t userpage_attributes =  MMU_PTE_BLOCK_XN | MMU_PTE_BLOCK_INNER_SHAREBLE | MMU_PTE_BLOCK_NS | ATTRIB_MEMTYPE_NORMAL;
            uintptr_t *mmu_l3_tbl = (uintptr_t *)TZRAM_GET_SEGMENT_ADDRESS(TZRAM_SEGMENT_ID_L3_TRANSLATION_TABLE);
            g_user_page_user_address = upage->user_address;
            mmu_map_page(mmu_l3_tbl, USER_PAGE_SECURE_MONITOR_ADDR, upage->user_address, userpage_attributes);
            tlb_invalidate_page_inner_shareable((void *)USER_PAGE_SECURE_MONITOR_ADDR);
            upage->secure_monitor_address = USER_PAGE_SECURE_MONITOR_ADDR;
        } else {
            generic_panic();
        }
    }

    return upage->secure_monitor_address != 0ull;
}

bool user_copy_to_secure(upage_ref_t *upage, void *secure_dst, void *user_src, size_t size) {
    /* Fail if the page doesn't match. */
    if (get_page_for_address(user_src) != upage->user_address) {
        return false;
    }

    /* Fail if we go past the page boundary. */
    if (size != 0 && get_page_for_address(user_src + size - 1) != upage->user_address) {
        return false;
    }

    void *secure_src = (void *)(upage->secure_monitor_address + ((uintptr_t)user_src - upage->user_address));
    if (size != 0) {
        memcpy(secure_dst, secure_src, size);
    }
    return true;
}

bool secure_copy_to_user(upage_ref_t *upage, void *user_dst, void *secure_src, size_t size) {
    /* Fail if the page doesn't match. */
    if (get_page_for_address(user_dst) != upage->user_address) {
        return false;
    }

    /* Fail if we go past the page boundary. */
    if (size != 0 && get_page_for_address(user_dst + size - 1) != upage->user_address) {
        return false;
    }

    void *secure_dst = (void *)(upage->secure_monitor_address + ((uintptr_t)user_dst - upage->user_address));
    if(size != 0) {
        memcpy(secure_dst, secure_src, size);
    }
    return true;
}
