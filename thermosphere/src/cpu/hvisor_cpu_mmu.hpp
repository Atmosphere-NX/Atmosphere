/*
 * Copyright (c) 2019-2020 Atmosphère-NX
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


#pragma once

#include "hvisor_cpu_sysreg_general.hpp"

namespace ams::hvisor::cpu {

    // Assumes addr is valid, must be called with interrupts masked
    inline uintptr_t Va2Pa(const void *vaddrEl2) {
        uintptr_t va = reinterpret_cast<uintptr_t>(vaddrEl2);
        __asm__ __volatile__("at s1e2r, %0" :: "r"(va) : "memory");
        return (THERMOSPHERE_GET_SYSREG(par_el1) & MASK2L(47, 12)) | (va & MASKL(12));
    }

    enum MmuPteType : u64 {
        MMU_ENTRY_FAULT  = 0,
        MMU_ENTRY_BLOCK  = 1,
        MMU_ENTRY_TABLE  = 3,

        // L3 (this definition allows for recursive page tables)
        MMU_ENTRY_PAGE   = 3,
    };

    // Multi-byte attributes...
    constexpr u64 MMU_ATTRINDEX(u64 idx) { return (idx & 8) << 2; }
    constexpr u64 MMU_MEMATTR(u64 attr) { return (attr & 0xF) << 2; }

    // Attributes. They are defined in a way that allows recursive page tables (assuming PBHA isn't used)
    enum MmuPteAttributes : u64 {
        // Stage 1 Table only, the rest is block/page only
        MMU_NS_TABLE        = BITL(62),
        MMU_AP_TABLE        = BITL(61),
        MMU_XN_TABLE        = BITL(60),
        MMU_PXN_TABLE       = BITL(59),

        MMU_UXN             = BITL(54), // EL1&0 only
        MMU_PXN             = BITL(53), // EL1&0 only
        MMU_XN              = MMU_UXN,
        MMU_XN0             = MMU_PXN,  // Armv8.2, stage 2 only
        MMU_CONTIGUOUS      = BITL(52),
        MMU_DBM             = BITL(51), // stage 1 only
        MMU_GP              = BITL(50), // undocumented

        // ARMv8.4-TTRem only
        MMU_NT              = BITL(16),

        // EL1&0 only
        MMU_NG              = BITL(11),

        MMU_AF              = BITL(10),

        // SH[1:0]
        MMU_NON_SHAREABLE   = 0 << 8,
        MMU_OUTER_SHAREABLE = 2 << 8,
        MMU_INNER_SHAREABLE = 2 << 8,

        // AP[2:1], stage 1 only. AP[0] does not exist.
        MMU_AP_PRIV_RW      = 0 << 6,
        MMU_AP_RW           = 1 << 6,
        MMU_AP_PRIV_RO      = 2 << 6,
        MMU_AP_RO           = 3 << 6,

        // S2AP[1:0], stage 2 only
        MMU_S2AP_NONE       = 0 << 6,
        MMU_S2AP_RO         = 1 << 6,
        MMU_S2AP_WO         = 2 << 6,
        MMU_S2AP_RW         = 3 << 6,

        // NS, stage 1 only
        MMU_NS              = BITL(5),

        // See above...

        // MemAttr[3:0], stage 2 only (convenience defs). When combining, strongest memory type applies
        MMU_MEMATTR_DEVICE_NGNRE    = MMU_MEMATTR(2),
        MMU_MEMATTR_UNCHANGED       = MMU_MEMATTR(0xF),

        // Other useful defines for stage 2:
        MMU_SAME_SHAREABILITY       = MMU_NON_SHAREABLE,
    };

    template<u32 Level, u32 AddressSpaceSize, bool IsMmuEnabled = false, TranslationGranuleSize GranuleSize = TranslationGranule_4K>
    class MmuTableBuilder final {
        private:
            static constexpr u32 tgBitSize = GetTranslationGranuleBitSize(GranuleSize);

            // tgBitSize - 3 = log2(tg / sizeof(u64))
            static constexpr u32 levelShift = tgBitSize + (tgBitSize - 3) * (3 - Level);
            static constexpr u32 levelBitSize = std::min(AddressSpaceSize - levelShift, tgBitSize - 3);
            static constexpr u64 levelMask = MASKL(levelBitSize);
            static constexpr size_t ComputeIndex(uintptr_t va)
            {
                return (va >> levelShift) & levelMask;
            }

        private:
            u64 *m_pageTable = nullptr;

        public:
            using NextLevelBuilder = MmuTableBuilder<Level + 1, AddressSpaceSize, IsMmuEnabled, GranuleSize>;
            static_assert(Level <= 3, "Invalid translation table level");
            static_assert(AddressSpaceSize <= 48);
            static_assert(AddressSpaceSize > levelShift, "Address space size mismatch with translation level");
            static constexpr size_t blockSize = BITL(levelShift);
            static constexpr size_t tableSize = BITL(levelBitSize);

        public:
            constexpr MmuTableBuilder(u64 *pageTable = nullptr) : m_pageTable{pageTable} {}

            constexpr MmuTableBuilder &InitializeTable()
            {
                std::memset(m_pageTable, 0, 8 * tableSize);
                // Fails to optimize before GCC 10: std::fill_n(m_pageTable, tableSize, MMU_ENTRY_FAULT);
                return *this;
            }

            // Precondition: va and pa bits in range
            constexpr NextLevelBuilder MapTable(uintptr_t va, uintptr_t pa, u64 *table, u64 attribs = 0) const
            {
                static_assert(Level < 3, "Level 3 is the last level of translation");

                m_pageTable[ComputeIndex(va)] = pa | attribs | MMU_ENTRY_TABLE;
                return NextLevelBuilder{table};
            }

            NextLevelBuilder MapTable(uintptr_t va, u64 *table, u64 attribs = 0) const
            {
                if constexpr (IsMmuEnabled) {
                    return MapTable(va, Va2Pa(table), table, attribs);
                } else {
                    return MapTable(va, reinterpret_cast<uintptr_t>(table), table, attribs);
                }
            }

            constexpr MmuTableBuilder &Unmap(uintptr_t va)
            {
                m_pageTable[ComputeIndex(va)] = MMU_ENTRY_FAULT;
                return *this;
            }

            // Precondition: guardSize == 0 if Level == 0
            constexpr MmuTableBuilder &UnmapRange(uintptr_t va, size_t size, size_t guardSize = 0)
            {
                for (size_t off = 0, offVa = 0; off < size; off += blockSize, offVa += blockSize + guardSize) {
                    Unmap(va + offVa);
                }
                return *this;
            }

            // Precondition: va and pa bits in range
            constexpr MmuTableBuilder &MapBlock(uintptr_t va, uintptr_t pa, u64 attribs)
            {
                static_assert(Level > 0, "Can only map L1 tables at L0");

                constexpr u64 entryType = Level == 3 ? MMU_ENTRY_PAGE : MMU_ENTRY_BLOCK;
                m_pageTable[ComputeIndex(va)] = pa | attribs | MMU_AF | entryType;
                return *this;
            }

            constexpr MmuTableBuilder &MapBlock(uintptr_t pa, u64 attribs)
            {
                return MapBlock(pa, pa, attribs);
            }

            // Precondition: size and guardSize are multiples of blockSize
            constexpr MmuTableBuilder &MapBlockRange(uintptr_t va, uintptr_t pa, size_t size, u64 attribs, size_t guardSize = 0)
            {
                for (size_t off = 0, offVa = 0; off < size; off += blockSize, offVa += blockSize + guardSize) {
                    MapBlock(va + offVa, pa + off, attribs);
                    UnmapRange(va + offVa + blockSize, guardSize, 0);
                }
                return *this;
            }

            constexpr MmuTableBuilder &MapBlockRange(uintptr_t pa, size_t size, u64 attribs)
            {
                return MapBlockRange(pa, pa, attribs, size, 0);
            }
    };

}
