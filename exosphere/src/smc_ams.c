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
#include "userpage.h"
#include "exocfg.h"

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

uint32_t ams_write_address(smc_args_t *args) {
    /* Implements a write to a DRAM page. */
    /* This operation can be used to write to read-only pages. */
    /* args->X[1] = Virtual address, must be size-bytes aligned and readable by EL0. */
    /* args->X[2] = Value. */
    /* args->X[3] = size (must be 1, 2, 4, or 8). */

    const uintptr_t el0_virtual_address = (uintptr_t)args->X[1];
    const uintptr_t dram_address = get_physical_address_el0(el0_virtual_address);
    const uintptr_t dram_page_offset = (dram_address & 0xFFFULL);
    const size_t size = (size_t)(args->X[3]);

    /* Validate addresses. */
    if (!ams_is_user_addr_valid(dram_address)) {
        return 2;
    }

    /* Validate size. */
    switch (size) {
        case 1:
        case 2:
        case 4:
        case 8:
            if ((size + dram_page_offset) > 0x1000) {
                return 2;
            }
            break;
        default:
            return 2;
    }

    /* Validate alignment. */
    if (dram_page_offset % size) {
        return 2;
    }

    /* Map pages. */
    ams_map_userpage(dram_address);

    /* Write data. */
    uintptr_t dram_ptr = (uintptr_t)(AMS_USER_PAGE_SECURE_MONITOR_ADDR + dram_page_offset);
    switch (size) {
        case 1:
            *((volatile uint8_t *)dram_ptr) = (uint8_t)(args->X[2]);
            break;
        case 2:
            *((volatile uint16_t *)dram_ptr) = (uint16_t)(args->X[2]);
            break;
        case 4:
            *((volatile uint32_t *)dram_ptr) = (uint32_t)(args->X[2]);
            break;
        case 8:
            *((volatile uint64_t *)dram_ptr) = args->X[2];
            break;
        default:
            generic_panic();
    }

    /* Flush! */
    flush_dcache_range((void *)dram_ptr, (void *)(dram_ptr + size));

    /* Unmap pages. */
    ams_unmap_userpage();

    return 0;
}

uint32_t ams_get_emummc_config(smc_args_t *args) {
    /* This retrieves configuration for the current emummc context. */
    /* args->X[1] = MMC id, must be size-bytes aligned and readable by EL0. */
    /* args->X[2] = Pointer to output (for paths for filebased + nintendo dir), must be at least 0x100 bytes. */
    const uint32_t mmc_id = (uint32_t)args->X[1];
    const uintptr_t dram_address = args->X[2];
    const uintptr_t dram_page_offset = (dram_address & 0xFFFULL);
    const exo_emummc_config_t *emummc_cfg = exosphere_get_emummc_config();

    if (mmc_id != EMUMMC_MMC_NAND) {
        /* Emummc config for non-NAND storage is not yet implemented. */
        return 1;
    }
    
    /* Require page alignment for input address. */
    if (!ams_is_user_addr_valid(dram_address) || dram_page_offset > 0x1000 - 0x100) {
        return 2;
    }

    /* Map pages. */
    ams_map_userpage(dram_address);
    
    void *user_address = (void *)(AMS_USER_PAGE_SECURE_MONITOR_ADDR + dram_page_offset);
    
    /* Copy redirection dir out to user. */
    memcpy((void *)((uintptr_t)user_address + sizeof(emummc_cfg->file_cfg)), emummc_cfg->emu_dir_path, sizeof(emummc_cfg->emu_dir_path));

    if (emummc_cfg->base_cfg.type == EMUMMC_TYPE_NONE) {
        /* Just copy base config. */
        memset(args, 0, sizeof(*args));
        memcpy(&args->X[1], emummc_cfg, sizeof(emummc_cfg->base_cfg));
        _Static_assert(sizeof(emummc_cfg->base_cfg) <= sizeof(*args) - sizeof(args->X[0]), "Emunand base config too big!");

        /* Unmap pages. */
        ams_unmap_userpage();
        return 0;
    } else if (emummc_cfg->base_cfg.type == EMUMMC_TYPE_PARTITION) {
        /* Copy base config and partition config. */
        memset(args, 0, sizeof(*args));
        memcpy(&args->X[1], emummc_cfg, sizeof(emummc_cfg->base_cfg) + sizeof(emummc_cfg->partition_cfg));
        _Static_assert(sizeof(emummc_cfg->base_cfg) + sizeof(emummc_cfg->partition_cfg) <= sizeof(*args) - sizeof(args->X[0]), "Emunand partition config too big!");

        /* Unmap pages. */
        ams_unmap_userpage();
        return 0;
    } else if (emummc_cfg->base_cfg.type == EMUMMC_TYPE_FILES) {
        /* Copy file dir path output to user. */
        memcpy(user_address, &emummc_cfg->file_cfg, sizeof(emummc_cfg->file_cfg));

        /* Copy base config afterwards, since this can't fail. */
        memset(args, 0, sizeof(*args));
        memcpy(&args->X[1], emummc_cfg, sizeof(emummc_cfg->base_cfg));
        _Static_assert(sizeof(emummc_cfg->base_cfg) <= sizeof(*args) - sizeof(args->X[0]), "Emunand base config too big!");

        /* Unmap pages. */
        ams_unmap_userpage();
        return 0;
    } else {
        /* Unmap pages. */
        ams_unmap_userpage();
        return 2;
    }
}