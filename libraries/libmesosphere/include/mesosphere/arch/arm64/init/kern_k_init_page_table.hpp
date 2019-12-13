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
#pragma once
#include <vapours.hpp>
#include <mesosphere/kern_panic.hpp>
#include <mesosphere/kern_k_typed_address.hpp>
#include "../kern_hardware_registers.hpp"

namespace ams::kern::init {

    constexpr size_t PageSize              = 0x1000;
    constexpr size_t L1BlockSize           = 0x40000000;
    constexpr size_t L2BlockSize           = 0x200000;
    constexpr size_t L2ContiguousBlockSize = 0x10 * L2BlockSize;
    constexpr size_t L3BlockSize           = 0x1000;
    constexpr size_t L3ContiguousBlockSize = 0x10 * L3BlockSize;

    class PageTableEntry {
        public:
            enum Permission : u64 {
                Permission_KernelRWX = ((0ul << 53) | (1ul << 54) | (0ul << 6)),
                Permission_KernelRX  = ((0ul << 53) | (1ul << 54) | (2ul << 6)),
                Permission_KernelR   = ((1ul << 53) | (1ul << 54) | (2ul << 6)),
                Permission_KernelRW  = ((1ul << 53) | (1ul << 54) | (0ul << 6)),

                Permission_UserRX    = ((1ul << 53) | (0ul << 54) | (3ul << 6)),
                Permission_UserR     = ((1ul << 53) | (1ul << 54) | (3ul << 6)),
                Permission_UserRW    = ((1ul << 53) | (1ul << 54) | (1ul << 6)),
            };

            enum Shareable : u64 {
                Shareable_NonShareable   = (0 << 8),
                Shareable_OuterShareable = (2 << 8),
                Shareable_InnerShareable = (3 << 8),
            };

            /* Official attributes are: */
            /*    0x00, 0x04, 0xFF, 0x44. 4-7 are unused. */
            enum PageAttribute : u64 {
                PageAttribute_Device_nGnRnE            = (0 << 2),
                PageAttribute_Device_nGnRE             = (1 << 2),
                PageAttribute_NormalMemory             = (2 << 2),
                PageAttribute_NormalMemoryNotCacheable = (3 << 2),
            };

            enum AccessFlag : u64 {
                AccessFlag_NotAccessed = (0 << 10),
                AccessFlag_Accessed    = (1 << 10),
            };
        protected:
            u64 attributes;
        public:
            /* Take in a raw attribute. */
            constexpr ALWAYS_INLINE PageTableEntry(u64 attr) : attributes(attr) { /* ... */ }

            /* Extend a previous attribute. */
            constexpr ALWAYS_INLINE PageTableEntry(const PageTableEntry &rhs, u64 new_attr) : attributes(rhs.attributes | new_attr) { /* ... */ }

            /* Construct a new attribute. */
            constexpr ALWAYS_INLINE PageTableEntry(Permission perm, PageAttribute p_a, Shareable share)
                : attributes(static_cast<u64>(perm) | static_cast<u64>(AccessFlag_Accessed) | static_cast<u64>(p_a) | static_cast<u64>(share))
            {
                /* ... */
            }
        protected:
            constexpr ALWAYS_INLINE u64 GetBits(size_t offset, size_t count) const {
                return (this->attributes >> offset) & ((1 << count) - 1);
            }

            constexpr ALWAYS_INLINE u64 SelectBits(size_t offset, size_t count) const {
                return this->attributes & (((1 << count) - 1) << offset);
            }
        public:
            constexpr ALWAYS_INLINE bool IsUserExecuteNever()        const { return this->GetBits(54, 1) != 0; }
            constexpr ALWAYS_INLINE bool IsPrivilegedExecuteNever()  const { return this->GetBits(53, 1) != 0; }
            constexpr ALWAYS_INLINE bool IsContiguous()              const { return this->GetBits(52, 1) != 0; }
            constexpr ALWAYS_INLINE AccessFlag GetAccessFlag()       const { return static_cast<AccessFlag>(this->GetBits(10, 1)); }
            constexpr ALWAYS_INLINE Shareable GetShareable()         const { return static_cast<Shareable>(this->GetBits(8, 2)); }
            constexpr ALWAYS_INLINE PageAttribute GetPageAttribute() const { return static_cast<PageAttribute>(this->GetBits(2, 3)); }
            constexpr ALWAYS_INLINE bool IsNonSecure()               const { return this->GetBits(5, 1) != 0; }
            constexpr ALWAYS_INLINE bool IsBlock()                   const { return this->GetBits(0, 2) == 0x1; }
            constexpr ALWAYS_INLINE bool IsTable()                   const { return this->GetBits(0, 2) == 0x3; }
    };

    static_assert(sizeof(PageTableEntry) == sizeof(u64));

    constexpr size_t MaxPageTableEntries = PageSize / sizeof(PageTableEntry);

    class L1PageTableEntry : public PageTableEntry {
        public:
            constexpr ALWAYS_INLINE L1PageTableEntry(KPhysicalAddress phys_addr, bool pxn)
                : PageTableEntry((0x3ul << 60) | (static_cast<u64>(pxn) << 59) | GetInteger(phys_addr) | 0x3)
            {
                /* ... */
            }
            constexpr ALWAYS_INLINE L1PageTableEntry(KPhysicalAddress phys_addr, const PageTableEntry &attr, bool contig)
                : PageTableEntry(attr, (static_cast<u64>(contig) << 52) | GetInteger(phys_addr) | 0x1)
            {
                /* ... */
            }

            constexpr ALWAYS_INLINE KPhysicalAddress GetBlock() const {
                return this->SelectBits(30, 18);
            }
            constexpr ALWAYS_INLINE KPhysicalAddress GetTable() const {
                return this->SelectBits(12, 36);
            }
    };

    class L2PageTableEntry : public PageTableEntry {
        public:
            constexpr ALWAYS_INLINE L2PageTableEntry(KPhysicalAddress phys_addr, bool pxn)
                : PageTableEntry((0x3ul << 60) | (static_cast<u64>(pxn) << 59) | GetInteger(phys_addr) | 0x3)
            {
                /* ... */
            }
            constexpr ALWAYS_INLINE L2PageTableEntry(KPhysicalAddress phys_addr, const PageTableEntry &attr, bool contig)
                : PageTableEntry(attr, (static_cast<u64>(contig) << 52) | GetInteger(phys_addr) | 0x1)
            {
                /* ... */
            }

            constexpr ALWAYS_INLINE KPhysicalAddress GetBlock() const {
                return this->SelectBits(21, 27);
            }
            constexpr ALWAYS_INLINE KPhysicalAddress GetTable() const {
                return this->SelectBits(12, 36);
            }
    };

    class L3PageTableEntry : public PageTableEntry {
        public:
            constexpr ALWAYS_INLINE L3PageTableEntry(KPhysicalAddress phys_addr, const PageTableEntry &attr, bool contig)
                : PageTableEntry(attr, (static_cast<u64>(contig) << 52) | GetInteger(phys_addr) | 0x1)
            {
                /* ... */
            }

            constexpr ALWAYS_INLINE KPhysicalAddress GetBlock() const {
                return this->SelectBits(21, 27);
            }
            constexpr ALWAYS_INLINE KPhysicalAddress GetTable() const {
                return this->SelectBits(12, 36);
            }
    };


    class KInitialPageTable {
        public:
            class IPageAllocator {
                public:
                    virtual KPhysicalAddress Allocate()           { return Null<KPhysicalAddress>; }
                    virtual void Free(KPhysicalAddress phys_addr) { /* Nothing to do here. */ (void)(phys_addr); }
            };
        private:
            KPhysicalAddress l1_table;
        public:
            constexpr ALWAYS_INLINE KInitialPageTable(KPhysicalAddress l1) : l1_table(l1) {
                ClearNewPageTable(this->l1_table);
            }
        private:
            static constexpr ALWAYS_INLINE L1PageTableEntry *GetL1Entry(KPhysicalAddress _l1_table, KVirtualAddress address) {
                L1PageTableEntry *l1_table = reinterpret_cast<L1PageTableEntry *>(GetInteger(_l1_table));
                return l1_table + ((GetInteger(address) >> 30) & (MaxPageTableEntries - 1));
            }

            static constexpr ALWAYS_INLINE L2PageTableEntry *GetL2Entry(const L1PageTableEntry *entry, KVirtualAddress address) {
                L2PageTableEntry *l2_table = reinterpret_cast<L2PageTableEntry *>(GetInteger(entry->GetTable()));
                return l2_table + ((GetInteger(address) >> 21) & (MaxPageTableEntries - 1));
            }

            static constexpr ALWAYS_INLINE L3PageTableEntry *GetL3Entry(const L2PageTableEntry *entry, KVirtualAddress address) {
                L3PageTableEntry *l3_table = reinterpret_cast<L3PageTableEntry *>(GetInteger(entry->GetTable()));
                return l3_table + ((GetInteger(address) >> 12) & (MaxPageTableEntries - 1));
            }

            static ALWAYS_INLINE void ClearNewPageTable(KPhysicalAddress address) {
                /* This Physical Address -> void * conversion is valid, because this is page table code. */
                /* The MMU is necessarily not yet turned on, if we are creating an initial page table. */
                std::memset(reinterpret_cast<void *>(GetInteger(address)), 0, PageSize);
            }
        public:
            void Map(KVirtualAddress virt_addr, size_t size, KPhysicalAddress phys_addr, const PageTableEntry &attr, IPageAllocator &allocator) {
                /* Ensure that addresses and sizes are page aligned. */
                MESOSPHERE_ABORT_UNLESS(util::IsAligned(GetInteger(virt_addr),    PageSize));
                MESOSPHERE_ABORT_UNLESS(util::IsAligned(GetInteger(phys_addr),    PageSize));
                MESOSPHERE_ABORT_UNLESS(util::IsAligned(size,                     PageSize));

                /* Iteratively map pages until the requested region is mapped. */
                while (size > 0) {
                    L1PageTableEntry *l1_entry = GetL1Entry(this->l1_table, virt_addr);

                    /* Can we make an L1 block? */
                    if (util::IsAligned(GetInteger(virt_addr), L1BlockSize) && util::IsAligned(GetInteger(phys_addr), L1BlockSize) && util::IsAligned(size, L1BlockSize)) {
                        *l1_entry = L1PageTableEntry(phys_addr, attr, false);
                        /* TODO: DataSynchronizationBarrier */
                        virt_addr += L1BlockSize;
                        phys_addr += L1BlockSize;
                        size      -= L1BlockSize;
                        continue;
                    }

                    /* If we don't already have an L2 table, we need to make a new one. */
                    if (!l1_entry->IsTable()) {
                        KPhysicalAddress new_table = allocator.Allocate();
                        ClearNewPageTable(new_table);
                        *l1_entry = L1PageTableEntry(phys_addr, attr.IsPrivilegedExecuteNever());
                    }

                    L2PageTableEntry *l2_entry = GetL2Entry(l1_entry, virt_addr);

                    /* Can we make a contiguous L2 block? */
                    if (util::IsAligned(GetInteger(virt_addr), L2ContiguousBlockSize) && util::IsAligned(GetInteger(phys_addr), L2ContiguousBlockSize) && util::IsAligned(size, L2ContiguousBlockSize)) {
                        for (size_t i = 0; i < L2ContiguousBlockSize / L2BlockSize; i++) {
                            l2_entry[i] = L2PageTableEntry(phys_addr, attr, true);
                            /* TODO: DataSynchronizationBarrier */
                            virt_addr += L2ContiguousBlockSize;
                            phys_addr += L2ContiguousBlockSize;
                            size      -= L2ContiguousBlockSize;
                        }
                        continue;
                    }

                    /* Can we make an L2 block? */
                    if (util::IsAligned(GetInteger(virt_addr), L2BlockSize) && util::IsAligned(GetInteger(phys_addr), L2BlockSize) && util::IsAligned(size, L2BlockSize)) {
                        *l2_entry = L2PageTableEntry(phys_addr, attr, false);
                        /* TODO: DataSynchronizationBarrier */
                        virt_addr += L2BlockSize;
                        phys_addr += L2BlockSize;
                        size      -= L2BlockSize;
                        continue;
                    }

                    /* If we don't already have an L3 table, we need to make a new one. */
                    if (!l2_entry->IsTable()) {
                        KPhysicalAddress new_table = allocator.Allocate();
                        ClearNewPageTable(new_table);
                        *l2_entry = L2PageTableEntry(phys_addr, attr.IsPrivilegedExecuteNever());
                    }

                    L3PageTableEntry *l3_entry = GetL3Entry(l2_entry, virt_addr);

                    /* Can we make a contiguous L3 block? */
                    if (util::IsAligned(GetInteger(virt_addr), L3ContiguousBlockSize) && util::IsAligned(GetInteger(phys_addr), L3ContiguousBlockSize) && util::IsAligned(size, L3ContiguousBlockSize)) {
                        for (size_t i = 0; i < L3ContiguousBlockSize / L3BlockSize; i++) {
                            l3_entry[i] = L3PageTableEntry(phys_addr, attr, true);
                            /* TODO: DataSynchronizationBarrier */
                            virt_addr += L3ContiguousBlockSize;
                            phys_addr += L3ContiguousBlockSize;
                            size      -= L3ContiguousBlockSize;
                        }
                        continue;
                    }

                    /* Make an L3 block. */
                    *l3_entry = L3PageTableEntry(phys_addr, attr, false);
                    /* TODO: DataSynchronizationBarrier */
                    virt_addr += L3BlockSize;
                    phys_addr += L3BlockSize;
                    size      -= L3BlockSize;
                }
            }

            bool IsFree(KVirtualAddress virt_addr, size_t size) {
                /* TODO */
                return false;
            }

            void ReprotectFromReadWriteToRead(KVirtualAddress virt_addr, size_t size) {
                /* TODO */
            }

    };

}
