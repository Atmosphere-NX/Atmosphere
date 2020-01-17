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

#include "stage2_config.h"
#include "interrupt_config.h"
#include "../../utils.h"
#include "../../mmu.h"
#include "../../core_ctx.h"

#define ADDRSPACESZ    36
#define ADDRSPACESZ2   ADDRSPACESZ

static TEMPORARY ALIGN(0x1000) u64 g_vttbl[BIT(ADDRSPACESZ2 - 30)] = {0};
static TEMPORARY ALIGN(0x1000) u64 g_vttbl_l2_mmio_0[512] = {0};
static TEMPORARY ALIGN(0x1000) u64 g_vttbl_l3_0[512] = {0};
static TEMPORARY uintptr_t g_vttblPaddr;

static inline void identityMapL1(u64 *tbl, uintptr_t addr, size_t size, u64 attribs)
{
    mmu_map_block_range(1, tbl, addr, addr, size, attribs | MMU_PTE_BLOCK_INNER_SHAREBLE);
}

static inline void identityMapL2(u64 *tbl, uintptr_t addr, size_t size, u64 attribs)
{
    mmu_map_block_range(2, tbl, addr, addr, size, attribs | MMU_PTE_BLOCK_INNER_SHAREBLE);
}

static inline void identityMapL3(u64 *tbl, uintptr_t addr, size_t size, u64 attribs)
{
    mmu_map_block_range(3, tbl, addr, addr, size, attribs | MMU_PTE_BLOCK_INNER_SHAREBLE);
}

uintptr_t stage2Configure(u32 *addrSpaceSize)
{
    *addrSpaceSize = ADDRSPACESZ2;
    static const u64 devattrs = MMU_PTE_BLOCK_XN | MMU_S2AP_RW | MMU_MEMATTR_DEVICE_NGNRE;
    static const u64 unchanged = MMU_S2AP_RW | MMU_MEMATTR_NORMAL_CACHEABLE_OR_UNCHANGED;
    if (currentCoreCtx->isBootCore) {
        g_vttblPaddr = va2pa(g_vttbl);
        uintptr_t *l2pa = (uintptr_t *)va2pa(g_vttbl_l2_mmio_0);
        uintptr_t *l3pa = (uintptr_t *)va2pa(g_vttbl_l3_0);

        identityMapL1(g_vttbl, 0x00000000ull, BITL(30), unchanged);
        identityMapL1(g_vttbl, 0x80000000ull, (BITL(ADDRSPACESZ2 - 30) - 2ull) << 30, unchanged);
        mmu_map_table(1, g_vttbl, 0x40000000ull, l2pa, 0);

        identityMapL2(g_vttbl_l2_mmio_0, 0x40000000ull, BITL(30), unchanged);
        mmu_map_table(2, g_vttbl_l2_mmio_0, 0x50000000ull, l3pa, 0);

        identityMapL3(g_vttbl_l3_0, 0x00000000ull, BITL(21), unchanged);

        // GICD -> trapped, GICv2 CPU -> vCPU interface, GICH -> trapped (access denied including for the unused view)
        mmu_unmap_page(g_vttbl_l3_0, MEMORY_MAP_PA_GICD);
        mmu_unmap_page(g_vttbl_l3_0, MEMORY_MAP_PA_GICH);
        mmu_unmap_page(g_vttbl_l3_0, MEMORY_MAP_PA_GICH + 0x1000ull);
        mmu_map_page_range(g_vttbl_l3_0, MEMORY_MAP_PA_GICC, MEMORY_MAP_PA_GICD, 0x2000ull, devattrs);
    }

    return (uintptr_t)g_vttbl;
}
