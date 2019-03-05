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
 
 
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "utils.h"
#include "smc_ams.h"
#include "arm.h"
#include "synchronization.h"
#include "memory_map.h"
#include "mmu.h"

static atomic_flag g_ams_userpage_mapped = ATOMIC_FLAG_INIT;
static atomic_flag g_ams_iram_page_mapped = ATOMIC_FLAG_INIT;

static inline uintptr_t get_ams_user_page_secure_monitor_addr(void) {
    return MMIO_GET_DEVICE_ADDRESS(MMIO_DEVID_AMS_USER_PAGE);
}

static inline uintptr_t get_ams_iram_page_secure_monitor_addr(void) {
    return MMIO_GET_DEVICE_ADDRESS(MMIO_DEVID_AMS_IRAM_PAGE);
}

#define AMS_USER_PAGE_SECURE_MONITOR_ADDR (get_ams_user_page_secure_monitor_addr())
#define AMS_IRAM_PAGE_SECURE_MONITOR_ADDR (get_ams_iram_page_secure_monitor_addr())


static inline uintptr_t get_page_for_address(void *address) {
    return ((uintptr_t)(address)) & ~0xFFFULL;
}

static bool ams_is_user_addr_valid(uintptr_t user_address) {
    /* Check that the address is in dram. */
    uintptr_t page_address = get_page_for_address((void *)user_address);
    return (page_address - 0x80000000ull) < (6ull << 30);
}

static bool ams_is_iram_addr_valid(uintptr_t iram_address) {
    /* Check that the address is in iram. */
    return 0x40000000ULL <= iram_address && iram_address <= 0x4003FFFFULL;
}

static void ams_map_userpage(uintptr_t user_address) {
    lock_acquire(&g_ams_userpage_mapped);
    static const uint64_t userpage_attributes =  MMU_PTE_BLOCK_XN | MMU_PTE_BLOCK_INNER_SHAREBLE | MMU_PTE_BLOCK_NS | ATTRIB_MEMTYPE_NORMAL;
    uintptr_t *mmu_l3_tbl = (uintptr_t *)TZRAM_GET_SEGMENT_ADDRESS(TZRAM_SEGMENT_ID_L3_TRANSLATION_TABLE);
    mmu_map_page(mmu_l3_tbl, AMS_USER_PAGE_SECURE_MONITOR_ADDR, get_page_for_address((void *)user_address), userpage_attributes);
    tlb_invalidate_page_inner_shareable((void *)AMS_USER_PAGE_SECURE_MONITOR_ADDR);
}

static void ams_unmap_userpage(void) {
    uintptr_t *mmu_l3_tbl = (uintptr_t *)TZRAM_GET_SEGMENT_ADDRESS(TZRAM_SEGMENT_ID_L3_TRANSLATION_TABLE);
    mmu_unmap_page(mmu_l3_tbl, AMS_USER_PAGE_SECURE_MONITOR_ADDR);
    tlb_invalidate_page_inner_shareable((void *)AMS_USER_PAGE_SECURE_MONITOR_ADDR);
    lock_release(&g_ams_userpage_mapped);
}

void ams_map_irampage(uintptr_t iram_address) {
    lock_acquire(&g_ams_iram_page_mapped);
    static const uint64_t irampage_attributes =  MMU_PTE_BLOCK_XN | MMU_PTE_BLOCK_INNER_SHAREBLE | ATTRIB_MEMTYPE_DEVICE;
    uintptr_t *mmu_l3_tbl = (uintptr_t *)TZRAM_GET_SEGMENT_ADDRESS(TZRAM_SEGMENT_ID_L3_TRANSLATION_TABLE);
    mmu_map_page(mmu_l3_tbl, AMS_IRAM_PAGE_SECURE_MONITOR_ADDR, get_page_for_address((void *)iram_address), irampage_attributes);
    tlb_invalidate_page_inner_shareable((void *)AMS_IRAM_PAGE_SECURE_MONITOR_ADDR);
}

void ams_unmap_irampage(void) {
    uintptr_t *mmu_l3_tbl = (uintptr_t *)TZRAM_GET_SEGMENT_ADDRESS(TZRAM_SEGMENT_ID_L3_TRANSLATION_TABLE);
    mmu_unmap_page(mmu_l3_tbl, AMS_IRAM_PAGE_SECURE_MONITOR_ADDR);
    tlb_invalidate_page_inner_shareable((void *)AMS_IRAM_PAGE_SECURE_MONITOR_ADDR);
    lock_release(&g_ams_iram_page_mapped);
}

uint32_t ams_iram_copy(smc_args_t *args) {
    /* Implements a DRAM <-> IRAM copy of up to one page. */
    /* This operation is necessary to implement reboot-to-payload. */
    /* args->X[1] = DRAM address (translated by kernel), must be 4-byte aligned. */
    /* args->X[2] = IRAM address, must be 4-byte aligned. */
    /* args->X[3] = size (must be <= 0x1000 and 4-byte aligned). */
    /* args->X[4] = 0 for read, 1 for write. */
    
    const uintptr_t dram_address = (uintptr_t)args->X[1];
    const uintptr_t iram_address = (uintptr_t)args->X[2];
    const uintptr_t dram_page_offset = (dram_address & 0xFFFULL);
    const uintptr_t iram_page_offset = (iram_address & 0xFFFULL);
    const size_t size = args->X[3];
    const uint32_t option = (uint32_t)args->X[4];
            
    /* Validate addresses. */
    if (!ams_is_user_addr_valid(dram_address) || !ams_is_iram_addr_valid(iram_address)) {
        return 2;
    }
    
    /* Validate size. */
    if (size > 0x1000 || (size + dram_page_offset) > 0x1000 || (size + iram_page_offset) > 0x1000) {
        return 2;
    }
    
    /* Validate alignment. */
    if (size % sizeof(uint32_t) || dram_page_offset % sizeof(uint32_t) || iram_page_offset % sizeof(uint32_t)) {
        return 2;
    }
    
    /* Validate argument. */
    if (option != 0 && option != 1) {
        return 2;
    }
    
    /* Map pages. */
    ams_map_userpage(dram_address);
    ams_map_irampage(iram_address);
    
    /* Set source/destination for copy. */
    volatile uint32_t *dram_ptr = (volatile uint32_t *)(AMS_USER_PAGE_SECURE_MONITOR_ADDR + dram_page_offset);
    volatile uint32_t *iram_ptr = (volatile uint32_t *)(AMS_IRAM_PAGE_SECURE_MONITOR_ADDR + iram_page_offset);
    
    volatile uint32_t *dst;
    volatile uint32_t *src;
    const size_t num_dwords = size / sizeof(uint32_t);
    if (option == 0) {
        dst = dram_ptr;
        src = iram_ptr;
    } else {
        dst = iram_ptr;
        src = dram_ptr;
    }
    
    /* Actually copy data. */
    for (size_t i = 0; i < num_dwords; i++) {
        dst[i] = src[i];
    }
    
    /* Flush! */
    flush_dcache_range((void *)dst, (void *)(dst + num_dwords));    
    
    /* Unmap pages. */
    ams_unmap_irampage();
    ams_unmap_userpage();
    
    return 0;
}
