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

#include "memory_map.h"
#include "../../utils.h"
#include "../../mmu.h"
#include "../../core_ctx.h"

// QEMU presently advertises 44-bit PAs we'll only use 39 of them to avoid level 0 tables.
#define ADDRSPACESZ    39
#define ADDRSPACESZ2   ADDRSPACESZ

static ALIGN(0x1000) u64 g_ttbl[BIT(ADDRSPACESZ - 30)] = {0};

static ALIGN(0x1000) u64 g_vttbl[BIT(ADDRSPACESZ2 - 30)] = {0};
static TEMPORARY ALIGN(0x1000) u64 g_vttbl_l2_mmio_0_0[512] = {0};
static TEMPORARY ALIGN(0x1000) u64 g_vttbl_l3_0[512] = {0};

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

uintptr_t configureMemoryMap(u32 *addrSpaceSize)
{
    // QEMU virt RAM address space starts at 0x40000000
    *addrSpaceSize = ADDRSPACESZ;
    if (currentCoreCtx->isBootCore && !currentCoreCtx->warmboot) {
        identityMapL1(g_ttbl, 0x00000000ull, BITL(30), ATTRIB_MEMTYPE_DEVICE);
        identityMapL1(g_ttbl, 0x40000000ull, (BITL(ADDRSPACESZ - 30) - 1ull) << 30, ATTRIB_MEMTYPE_NORMAL);
    }

    return (uintptr_t)g_ttbl;
}

uintptr_t configureStage2MemoryMap(u32 *addrSpaceSize)
{
    *addrSpaceSize = ADDRSPACESZ2;
    static const u64 devattrs = MMU_S2AP_RW | MMU_MEMATTR_DEVICE_NGNRE;
    static const u64 unchanged = MMU_S2AP_RW | MMU_MEMATTR_NORMAL_CACHEABLE_OR_UNCHANGED;

    if (currentCoreCtx->isBootCore) {
        identityMapL1(g_vttbl, 0, 4ull << 30, unchanged);
        identityMapL1(g_vttbl, 0x40000000ull, (BITL(ADDRSPACESZ2 - 30) - 1ull) << 30, unchanged);
        mmu_map_table(1, g_vttbl, 0x00000000ull, g_vttbl_l2_mmio_0_0, 0);

        identityMapL2(g_vttbl_l2_mmio_0_0, 0x08000000ull, BITL(30), unchanged);
        mmu_map_table(2, g_vttbl_l2_mmio_0_0, 0x08000000ull, g_vttbl_l3_0, 0);

        identityMapL3(g_vttbl_l3_0, 0x08000000ull, BITL(21), unchanged);

        // GICv2 CPU -> vCPU interface
        mmu_map_page_range(g_vttbl_l3_0, 0x08010000ull, 0x08040000ull, 0x10000ull, devattrs);
    }

    return (uintptr_t)g_vttbl;
}
