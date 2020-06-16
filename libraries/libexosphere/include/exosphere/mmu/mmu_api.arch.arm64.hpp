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
#pragma once
#include <vapours.hpp>

namespace ams::mmu::arch::arm64 {

    enum PageTableTableAttribute : u64 {
        PageTableTableAttribute_None                   = (0ul <<  0),
        PageTableTableAttribute_PrivilegedExecuteNever = (1ul << 59),
        PageTableTableAttribute_ExecuteNever           = (1ul << 60),
        PageTableTableAttribute_NonSecure              = (1ul << 63),

        PageTableTableAttributes_El3SecureCode          = PageTableTableAttribute_None,
        PageTableTableAttributes_El3SecureData          = PageTableTableAttribute_ExecuteNever,
        PageTableTableAttributes_El3NonSecureCode       = PageTableTableAttributes_El3SecureCode | PageTableTableAttribute_NonSecure,
        PageTableTableAttributes_El3NonSecureData       = PageTableTableAttributes_El3SecureData | PageTableTableAttribute_NonSecure,
    };

    enum PageTableMappingAttribute : u64{
        /* Security. */
        PageTableMappingAttribute_NonSecure = (1ul << 5),

        /* El1 Access. */
        PageTableMappingAttribute_El1NotAllowed = (0ul << 6),
        PageTableMappingAttribute_El1Allowed    = (1ul << 6),

        /* RW Permission. */
        PageTableMappingAttribute_PermissionReadWrite = (0ul << 7),
        PageTableMappingAttribute_PermissionReadOnly  = (1ul << 7),

        /* Shareability. */
        PageTableMappingAttribute_ShareabilityNonShareable   = (0ul << 8),
        PageTableMappingAttribute_ShareabiltiyOuterShareable = (2ul << 8),
        PageTableMappingAttribute_ShareabilityInnerShareable = (3ul << 8),

        /* Access flag. */
        PageTableMappingAttribute_AccessFlagNotAccessed = (0ul << 10),
        PageTableMappingAttribute_AccessFlagAccessed    = (1ul << 10),

        /* Global. */
        PageTableMappingAttribute_Global    = (0ul << 11),
        PageTableMappingAttribute_NonGlobal = (1ul << 11),

        /* Contiguous */
        PageTableMappingAttribute_NonContiguous          = (0ul << 52),
        PageTableMappingAttribute_Contiguous             = (1ul << 52),

        /* Privileged Execute Never */
        PageTableMappingAttribute_PrivilegedExecuteNever = (1ul << 53),

        /* Execute Never */
        PageTableMappingAttribute_ExecuteNever = (1ul << 54),


        /* Useful definitions. */
        PageTableMappingAttributes_El3SecureRwCode = (
            PageTableMappingAttribute_PermissionReadWrite       |
            PageTableMappingAttribute_ShareabilityInnerShareable
        ),

        PageTableMappingAttributes_El3SecureRoCode = (
            PageTableMappingAttribute_PermissionReadOnly        |
            PageTableMappingAttribute_ShareabilityInnerShareable
        ),

        PageTableMappingAttributes_El3SecureRoData = (
            PageTableMappingAttribute_PermissionReadOnly         |
            PageTableMappingAttribute_ShareabilityInnerShareable |
            PageTableMappingAttribute_ExecuteNever
        ),

        PageTableMappingAttributes_El3SecureRwData = (
            PageTableMappingAttribute_PermissionReadWrite        |
            PageTableMappingAttribute_ShareabilityInnerShareable |
            PageTableMappingAttribute_ExecuteNever
        ),

        PageTableMappingAttributes_El3NonSecureRwCode = PageTableMappingAttributes_El3SecureRwCode | PageTableMappingAttribute_NonSecure,
        PageTableMappingAttributes_El3NonSecureRoCode = PageTableMappingAttributes_El3SecureRoCode | PageTableMappingAttribute_NonSecure,
        PageTableMappingAttributes_El3NonSecureRoData = PageTableMappingAttributes_El3SecureRoData | PageTableMappingAttribute_NonSecure,
        PageTableMappingAttributes_El3NonSecureRwData = PageTableMappingAttributes_El3SecureRwData | PageTableMappingAttribute_NonSecure,


        PageTableMappingAttributes_El3SecureDevice    = PageTableMappingAttributes_El3SecureRwData,
        PageTableMappingAttributes_El3NonSecureDevice = PageTableMappingAttributes_El3NonSecureRwData,
    };

    enum MemoryRegionAttribute : u64 {
        MemoryRegionAttribute_Device_nGnRnE            = (0ul << 2),
        MemoryRegionAttribute_Device_nGnRE             = (1ul << 2),
        MemoryRegionAttribute_NormalMemory             = (2ul << 2),
        MemoryRegionAttribute_NormalMemoryNotCacheable = (3ul << 2),

        MemoryRegionAttribute_NormalInnerShift = 0,
        MemoryRegionAttribute_NormalOuterShift = 4,

        #define AMS_MRA_DEFINE_NORMAL_ATTR(__NAME__, __VAL__)                                                  \
            MemoryRegionAttribute_NormalInner##__NAME__ = (__VAL__ << MemoryRegionAttribute_NormalInnerShift), \
            MemoryRegionAttribute_NormalOuter##__NAME__ = (__VAL__ << MemoryRegionAttribute_NormalOuterShift)

        AMS_MRA_DEFINE_NORMAL_ATTR(NonCacheable, 4),

        AMS_MRA_DEFINE_NORMAL_ATTR(WriteAllocate, (1ul << 0)),
        AMS_MRA_DEFINE_NORMAL_ATTR(ReadAllocate,  (1ul << 1)),

        AMS_MRA_DEFINE_NORMAL_ATTR(WriteThroughTransient,    (0ul << 2)),
        AMS_MRA_DEFINE_NORMAL_ATTR(WriteBackTransient,       (1ul << 2)),
        AMS_MRA_DEFINE_NORMAL_ATTR(WriteThroughNonTransient, (2ul << 2)),
        AMS_MRA_DEFINE_NORMAL_ATTR(WriteBackNonTransient,    (3ul << 2)),

        #undef AMS_MRA_DEFINE_NORMAL_ATTR

        MemoryRegionAttributes_Normal = (
            MemoryRegionAttribute_NormalInnerReadAllocate          |
            MemoryRegionAttribute_NormalOuterReadAllocate          |
            MemoryRegionAttribute_NormalInnerWriteAllocate         |
            MemoryRegionAttribute_NormalOuterWriteAllocate         |
            MemoryRegionAttribute_NormalInnerWriteBackNonTransient |
            MemoryRegionAttribute_NormalOuterWriteBackNonTransient
        ),

        MemoryRegionAttributes_Device = (
            MemoryRegionAttribute_Device_nGnRE
        ),
    };

    constexpr inline u64 MemoryRegionAttributeWidth = 8;

    constexpr ALWAYS_INLINE PageTableMappingAttribute AddMappingAttributeIndex(PageTableMappingAttribute attr, int index) {
        return static_cast<PageTableMappingAttribute>(attr | (static_cast<typename std::underlying_type<PageTableMappingAttribute>::type>(index) << 2));
    }

    constexpr inline u64 L1EntryShift = 30;
    constexpr inline u64 L2EntryShift = 21;
    constexpr inline u64 L3EntryShift = 12;

    constexpr inline u64 L1EntrySize = 1_GB;
    constexpr inline u64 L2EntrySize = 2_MB;
    constexpr inline u64 L3EntrySize = 4_KB;

    constexpr inline u64 PageSize = L3EntrySize;

    constexpr inline u64 L1EntryMask = ((1ul << (48 - L1EntryShift)) - 1) << L1EntryShift;
    constexpr inline u64 L2EntryMask = ((1ul << (48 - L2EntryShift)) - 1) << L2EntryShift;
    constexpr inline u64 L3EntryMask = ((1ul << (48 - L3EntryShift)) - 1) << L3EntryShift;

    constexpr inline u64 TableEntryMask = L3EntryMask;

    static_assert(L1EntryMask == 0x0000FFFFC0000000ul);
    static_assert(L2EntryMask == 0x0000FFFFFFE00000ul);
    static_assert(L3EntryMask == 0x0000FFFFFFFFF000ul);

    constexpr inline u64 TableEntryIndexMask = 0x1FF;

    constexpr inline u64 EntryBlock = 0x1ul;
    constexpr inline u64 EntryPage  = 0x3ul;

    constexpr ALWAYS_INLINE u64 MakeTableEntry(u64 address, PageTableTableAttribute attr) {
        return address | static_cast<u64>(attr) | 0x3ul;
    }

    constexpr ALWAYS_INLINE u64 MakeL1BlockEntry(u64 address, PageTableMappingAttribute attr) {
        return address | static_cast<u64>(attr) | static_cast<u64>(PageTableMappingAttribute_AccessFlagAccessed) | 0x1ul;
    }

    constexpr ALWAYS_INLINE u64 MakeL2BlockEntry(u64 address, PageTableMappingAttribute attr) {
        return address | static_cast<u64>(attr) | static_cast<u64>(PageTableMappingAttribute_AccessFlagAccessed) | 0x1ul;
    }

    constexpr ALWAYS_INLINE u64 MakeL3BlockEntry(u64 address, PageTableMappingAttribute attr) {
        return address | static_cast<u64>(attr) | static_cast<u64>(PageTableMappingAttribute_AccessFlagAccessed) | 0x3ul;
    }

    constexpr ALWAYS_INLINE uintptr_t GetL2Offset(uintptr_t address) {
        return address & ((1ul << L2EntryShift) - 1);
    }

    constexpr ALWAYS_INLINE u64 GetL1EntryIndex(uintptr_t address) {
        return ((address >> L1EntryShift) & TableEntryIndexMask);
    }

    constexpr ALWAYS_INLINE u64 GetL2EntryIndex(uintptr_t address) {
        return ((address >> L2EntryShift) & TableEntryIndexMask);
    }

    constexpr ALWAYS_INLINE u64 GetL3EntryIndex(uintptr_t address) {
        return ((address >> L3EntryShift) & TableEntryIndexMask);
    }

    constexpr ALWAYS_INLINE void SetTableEntryImpl(volatile u64 *table, u64 index, u64 value) {
        /* Write the value. */
        table[index] = value;
    }

    constexpr ALWAYS_INLINE void SetTableEntry(u64 *table, u64 index, u64 value) {
        /* Ensure (for constexpr validation purposes) that the entry we set is clear. */
        if (std::is_constant_evaluated()) {
            if (table[index]) {
                __builtin_unreachable();
            }
        }

        /* Set the value. */
        SetTableEntryImpl(table, index, value);
    }

    constexpr ALWAYS_INLINE void SetL1TableEntry(u64 *table, uintptr_t virt_addr, uintptr_t phys_addr, PageTableTableAttribute attr) {
        SetTableEntry(table, GetL1EntryIndex(virt_addr), MakeTableEntry(phys_addr & TableEntryMask, attr));
    }

    constexpr ALWAYS_INLINE void SetL2TableEntry(u64 *table, uintptr_t virt_addr, uintptr_t phys_addr, PageTableTableAttribute attr) {
        SetTableEntry(table, GetL2EntryIndex(virt_addr), MakeTableEntry(phys_addr & TableEntryMask, attr));
    }

    constexpr ALWAYS_INLINE void SetL1BlockEntry(u64 *table, uintptr_t virt_addr, uintptr_t phys_addr, size_t size, PageTableMappingAttribute attr) {
        const u64 start = GetL1EntryIndex(virt_addr);
        const u64 count = (size >> L1EntryShift);

        for (u64 i = 0; i < count; ++i) {
            SetTableEntry(table, start + i, MakeL1BlockEntry((phys_addr & L1EntryMask) + (i << L1EntryShift), attr));
        }
    }

    constexpr ALWAYS_INLINE void SetL2BlockEntry(u64 *table, uintptr_t virt_addr, uintptr_t phys_addr, size_t size, PageTableMappingAttribute attr) {
        const u64 start = GetL2EntryIndex(virt_addr);
        const u64 count = (size >> L2EntryShift);

        for (u64 i = 0; i < count; ++i) {
            SetTableEntry(table, start + i, MakeL2BlockEntry((phys_addr & L2EntryMask) + (i << L2EntryShift), attr));
        }
    }

    constexpr ALWAYS_INLINE void SetL3BlockEntry(u64 *table, uintptr_t virt_addr, uintptr_t phys_addr, size_t size, PageTableMappingAttribute attr) {
        const u64 start = GetL3EntryIndex(virt_addr);
        const u64 count = (size >> L3EntryShift);

        for (u64 i = 0; i < count; ++i) {
            SetTableEntry(table, start + i, MakeL3BlockEntry((phys_addr & L3EntryMask) + (i << L3EntryShift), attr));
        }
    }

    constexpr ALWAYS_INLINE void InvalidateL1Entries(volatile u64 *table, uintptr_t virt_addr, size_t size) {
        const u64 start = GetL1EntryIndex(virt_addr);
        const u64 count = (size >> L1EntryShift);
        const u64 end   = start + count;

        for (u64 i = start; i < end; ++i) {
            table[i] = 0;
        }
    }

    constexpr ALWAYS_INLINE void InvalidateL2Entries(volatile u64 *table, uintptr_t virt_addr, size_t size) {
        const u64 start = GetL2EntryIndex(virt_addr);
        const u64 count = (size >> L2EntryShift);
        const u64 end   = start + count;

        for (u64 i = start; i < end; ++i) {
            table[i] = 0;
        }
    }

    constexpr ALWAYS_INLINE void InvalidateL3Entries(volatile u64 *table, uintptr_t virt_addr, size_t size) {
        const u64 start = GetL3EntryIndex(virt_addr);
        const u64 count = (size >> L3EntryShift);
        const u64 end   = start + count;

        for (u64 i = start; i < end; ++i) {
            table[i] = 0;
        }
    }

}
