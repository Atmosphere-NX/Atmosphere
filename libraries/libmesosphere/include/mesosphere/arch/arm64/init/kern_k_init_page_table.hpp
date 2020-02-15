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
#include <mesosphere/kern_common.hpp>
#include <mesosphere/kern_k_typed_address.hpp>
#include <mesosphere/kern_select_cpu.hpp>
#include <mesosphere/arch/arm64/kern_k_page_table_entry.hpp>

namespace ams::kern::arch::arm64::init {

    class KInitialPageTable {
        public:
            class IPageAllocator {
                public:
                    virtual KPhysicalAddress Allocate()           { return Null<KPhysicalAddress>; }
                    virtual void Free(KPhysicalAddress phys_addr) { /* Nothing to do here. */ (void)(phys_addr); }
            };

            struct NoClear{};
        private:
            KPhysicalAddress l1_table;
        public:
            constexpr ALWAYS_INLINE KInitialPageTable(KPhysicalAddress l1, NoClear) : l1_table(l1) { /* ... */ }

            constexpr ALWAYS_INLINE KInitialPageTable(KPhysicalAddress l1) : KInitialPageTable(l1, NoClear{}) {
                ClearNewPageTable(this->l1_table);
            }

            constexpr ALWAYS_INLINE uintptr_t GetL1TableAddress() const {
                return GetInteger(this->l1_table);
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
            void NOINLINE Map(KVirtualAddress virt_addr, size_t size, KPhysicalAddress phys_addr, const PageTableEntry &attr, IPageAllocator &allocator) {
                /* Ensure that addresses and sizes are page aligned. */
                MESOSPHERE_INIT_ABORT_UNLESS(util::IsAligned(GetInteger(virt_addr),    PageSize));
                MESOSPHERE_INIT_ABORT_UNLESS(util::IsAligned(GetInteger(phys_addr),    PageSize));
                MESOSPHERE_INIT_ABORT_UNLESS(util::IsAligned(size,                     PageSize));

                /* Iteratively map pages until the requested region is mapped. */
                while (size > 0) {
                    L1PageTableEntry *l1_entry = GetL1Entry(this->l1_table, virt_addr);

                    /* Can we make an L1 block? */
                    if (util::IsAligned(GetInteger(virt_addr), L1BlockSize) && util::IsAligned(GetInteger(phys_addr), L1BlockSize) && util::IsAligned(size, L1BlockSize)) {
                        *l1_entry = L1PageTableEntry(phys_addr, attr, false);
                        cpu::DataSynchronizationBarrierInnerShareable();

                        virt_addr += L1BlockSize;
                        phys_addr += L1BlockSize;
                        size      -= L1BlockSize;
                        continue;
                    }

                    /* If we don't already have an L2 table, we need to make a new one. */
                    if (!l1_entry->IsTable()) {
                        KPhysicalAddress new_table = allocator.Allocate();
                        ClearNewPageTable(new_table);
                        *l1_entry = L1PageTableEntry(new_table, attr.IsPrivilegedExecuteNever());
                        cpu::DataSynchronizationBarrierInnerShareable();
                    }

                    L2PageTableEntry *l2_entry = GetL2Entry(l1_entry, virt_addr);

                    /* Can we make a contiguous L2 block? */
                    if (util::IsAligned(GetInteger(virt_addr), L2ContiguousBlockSize) && util::IsAligned(GetInteger(phys_addr), L2ContiguousBlockSize) && util::IsAligned(size, L2ContiguousBlockSize)) {
                        for (size_t i = 0; i < L2ContiguousBlockSize / L2BlockSize; i++) {
                            l2_entry[i] = L2PageTableEntry(phys_addr, attr, true);
                            cpu::DataSynchronizationBarrierInnerShareable();

                            virt_addr += L2BlockSize;
                            phys_addr += L2BlockSize;
                            size      -= L2BlockSize;
                        }
                        continue;
                    }

                    /* Can we make an L2 block? */
                    if (util::IsAligned(GetInteger(virt_addr), L2BlockSize) && util::IsAligned(GetInteger(phys_addr), L2BlockSize) && util::IsAligned(size, L2BlockSize)) {
                        *l2_entry = L2PageTableEntry(phys_addr, attr, false);
                        cpu::DataSynchronizationBarrierInnerShareable();

                        virt_addr += L2BlockSize;
                        phys_addr += L2BlockSize;
                        size      -= L2BlockSize;
                        continue;
                    }

                    /* If we don't already have an L3 table, we need to make a new one. */
                    if (!l2_entry->IsTable()) {
                        KPhysicalAddress new_table = allocator.Allocate();
                        ClearNewPageTable(new_table);
                        *l2_entry = L2PageTableEntry(new_table, attr.IsPrivilegedExecuteNever());
                        cpu::DataSynchronizationBarrierInnerShareable();
                    }

                    L3PageTableEntry *l3_entry = GetL3Entry(l2_entry, virt_addr);

                    /* Can we make a contiguous L3 block? */
                    if (util::IsAligned(GetInteger(virt_addr), L3ContiguousBlockSize) && util::IsAligned(GetInteger(phys_addr), L3ContiguousBlockSize) && util::IsAligned(size, L3ContiguousBlockSize)) {
                        for (size_t i = 0; i < L3ContiguousBlockSize / L3BlockSize; i++) {
                            l3_entry[i] = L3PageTableEntry(phys_addr, attr, true);
                            cpu::DataSynchronizationBarrierInnerShareable();

                            virt_addr += L3BlockSize;
                            phys_addr += L3BlockSize;
                            size      -= L3BlockSize;
                        }
                        continue;
                    }

                    /* Make an L3 block. */
                    *l3_entry = L3PageTableEntry(phys_addr, attr, false);
                    cpu::DataSynchronizationBarrierInnerShareable();
                    virt_addr += L3BlockSize;
                    phys_addr += L3BlockSize;
                    size      -= L3BlockSize;
                }
            }

            KPhysicalAddress GetPhysicalAddress(KVirtualAddress virt_addr) const {
                /* Get the L1 entry. */
                const L1PageTableEntry *l1_entry = GetL1Entry(this->l1_table, virt_addr);

                if (l1_entry->IsBlock()) {
                    return l1_entry->GetBlock() + (GetInteger(virt_addr) & (L1BlockSize - 1));
                }

                MESOSPHERE_INIT_ABORT_UNLESS(l1_entry->IsTable());

                /* Get the L2 entry. */
                const L2PageTableEntry *l2_entry = GetL2Entry(l1_entry, virt_addr);

                if (l2_entry->IsBlock()) {
                    return l2_entry->GetBlock() + (GetInteger(virt_addr) & (L2BlockSize - 1));
                }

                MESOSPHERE_INIT_ABORT_UNLESS(l2_entry->IsTable());

                /* Get the L3 entry. */
                const L3PageTableEntry *l3_entry = GetL3Entry(l2_entry, virt_addr);

                MESOSPHERE_INIT_ABORT_UNLESS(l3_entry->IsBlock());

                return l3_entry->GetBlock() + (GetInteger(virt_addr) & (L3BlockSize - 1));
            }

            bool IsFree(KVirtualAddress virt_addr, size_t size) {
                /* Ensure that addresses and sizes are page aligned. */
                MESOSPHERE_INIT_ABORT_UNLESS(util::IsAligned(GetInteger(virt_addr),    PageSize));
                MESOSPHERE_INIT_ABORT_UNLESS(util::IsAligned(size,                     PageSize));

                const KVirtualAddress end_virt_addr = virt_addr + size;
                while (virt_addr < end_virt_addr) {
                    L1PageTableEntry *l1_entry = GetL1Entry(this->l1_table, virt_addr);

                    /* If an L1 block is mapped, the address isn't free. */
                    if (l1_entry->IsBlock()) {
                        return false;
                    }

                    if (!l1_entry->IsTable()) {
                        /* Not a table, so just move to check the next region. */
                        virt_addr = util::AlignDown(GetInteger(virt_addr) + L1BlockSize, L1BlockSize);
                        continue;
                    }

                    /* Table, so check if we're mapped in L2. */
                    L2PageTableEntry *l2_entry = GetL2Entry(l1_entry, virt_addr);

                    if (l2_entry->IsBlock()) {
                        return false;
                    }

                    if (!l2_entry->IsTable()) {
                        /* Not a table, so just move to check the next region. */
                        virt_addr = util::AlignDown(GetInteger(virt_addr) + L2BlockSize, L2BlockSize);
                        continue;
                    }

                    /* Table, so check if we're mapped in L3. */
                    L3PageTableEntry *l3_entry = GetL3Entry(l2_entry, virt_addr);

                    if (l3_entry->IsBlock()) {
                        return false;
                    }

                    /* Not a block, so move on to check the next page. */
                    virt_addr = util::AlignDown(GetInteger(virt_addr) + L3BlockSize, L3BlockSize);
                }
                return true;
            }

            void Reprotect(KVirtualAddress virt_addr, size_t size, const PageTableEntry &attr_before, const PageTableEntry &attr_after) {
                /* Ensure data consistency before we begin reprotection. */
                cpu::DataSynchronizationBarrierInnerShareable();

                /* Ensure that addresses and sizes are page aligned. */
                MESOSPHERE_INIT_ABORT_UNLESS(util::IsAligned(GetInteger(virt_addr),    PageSize));
                MESOSPHERE_INIT_ABORT_UNLESS(util::IsAligned(size,                     PageSize));

                /* Iteratively reprotect pages until the requested region is reprotected. */
                while (size > 0) {
                    L1PageTableEntry *l1_entry = GetL1Entry(this->l1_table, virt_addr);

                    /* Check if an L1 block is present. */
                    if (l1_entry->IsBlock()) {
                        /* Ensure that we are allowed to have an L1 block here. */
                        const KPhysicalAddress block = l1_entry->GetBlock();
                        MESOSPHERE_INIT_ABORT_UNLESS(util::IsAligned(GetInteger(virt_addr), L1BlockSize));
                        MESOSPHERE_INIT_ABORT_UNLESS(util::IsAligned(size, L1BlockSize));
                        MESOSPHERE_INIT_ABORT_UNLESS(l1_entry->IsCompatibleWithAttribute(attr_before, false));

                        /* Invalidate the existing L1 block. */
                        *static_cast<PageTableEntry *>(l1_entry) = InvalidPageTableEntry;
                        cpu::DataSynchronizationBarrierInnerShareable();
                        cpu::InvalidateEntireTlb();

                        /* Create new L1 block. */
                        *l1_entry = L1PageTableEntry(block, attr_after, false);

                        virt_addr += L1BlockSize;
                        size      -= L1BlockSize;
                        continue;
                    }

                    /* Not a block, so we must be a table. */
                    MESOSPHERE_INIT_ABORT_UNLESS(l1_entry->IsTable());

                    L2PageTableEntry *l2_entry = GetL2Entry(l1_entry, virt_addr);
                    if (l2_entry->IsBlock()) {
                        const KPhysicalAddress block = l2_entry->GetBlock();

                        if (l2_entry->IsContiguous()) {
                            /* Ensure that we are allowed to have a contiguous L2 block here. */
                            MESOSPHERE_INIT_ABORT_UNLESS(util::IsAligned(GetInteger(virt_addr), L2ContiguousBlockSize));
                            MESOSPHERE_INIT_ABORT_UNLESS(util::IsAligned(GetInteger(block),     L2ContiguousBlockSize));
                            MESOSPHERE_INIT_ABORT_UNLESS(util::IsAligned(size,                  L2ContiguousBlockSize));

                            /* Invalidate the existing contiguous L2 block. */
                            for (size_t i = 0; i < L2ContiguousBlockSize / L2BlockSize; i++) {
                                /* Ensure that the entry is valid. */
                                MESOSPHERE_INIT_ABORT_UNLESS(l2_entry[i].IsCompatibleWithAttribute(attr_before, true));
                                static_cast<PageTableEntry *>(l2_entry)[i] = InvalidPageTableEntry;
                            }
                            cpu::DataSynchronizationBarrierInnerShareable();
                            cpu::InvalidateEntireTlb();

                            /* Create a new contiguous L2 block. */
                            for (size_t i = 0; i < L2ContiguousBlockSize / L2BlockSize; i++) {
                                l2_entry[i] = L2PageTableEntry(block + L2BlockSize * i, attr_after, true);
                            }

                            virt_addr += L2ContiguousBlockSize;
                            size      -= L2ContiguousBlockSize;
                        } else {
                            /* Ensure that we are allowed to have an L2 block here. */
                            MESOSPHERE_INIT_ABORT_UNLESS(util::IsAligned(GetInteger(virt_addr), L2BlockSize));
                            MESOSPHERE_INIT_ABORT_UNLESS(util::IsAligned(GetInteger(block),     L2BlockSize));
                            MESOSPHERE_INIT_ABORT_UNLESS(util::IsAligned(size,                  L2BlockSize));
                            MESOSPHERE_INIT_ABORT_UNLESS(l2_entry->IsCompatibleWithAttribute(attr_before, false));

                            /* Invalidate the existing L2 block. */
                            *static_cast<PageTableEntry *>(l2_entry) = InvalidPageTableEntry;
                            cpu::DataSynchronizationBarrierInnerShareable();
                            cpu::InvalidateEntireTlb();

                            /* Create new L2 block. */
                            *l2_entry = L2PageTableEntry(block, attr_after, false);

                            virt_addr += L2BlockSize;
                            size      -= L2BlockSize;
                        }

                        continue;
                    }

                    /* Not a block, so we must be a table. */
                    MESOSPHERE_INIT_ABORT_UNLESS(l2_entry->IsTable());

                    /* We must have a mapped l3 entry to reprotect. */
                    L3PageTableEntry *l3_entry = GetL3Entry(l2_entry, virt_addr);
                    MESOSPHERE_INIT_ABORT_UNLESS(l3_entry->IsBlock());
                    const KPhysicalAddress block = l3_entry->GetBlock();

                    if (l3_entry->IsContiguous()) {
                        /* Ensure that we are allowed to have a contiguous L3 block here. */
                        MESOSPHERE_INIT_ABORT_UNLESS(util::IsAligned(GetInteger(virt_addr), L3ContiguousBlockSize));
                        MESOSPHERE_INIT_ABORT_UNLESS(util::IsAligned(GetInteger(block),     L3ContiguousBlockSize));
                        MESOSPHERE_INIT_ABORT_UNLESS(util::IsAligned(size,                  L3ContiguousBlockSize));

                        /* Invalidate the existing contiguous L3 block. */
                        for (size_t i = 0; i < L3ContiguousBlockSize / L3BlockSize; i++) {
                            /* Ensure that the entry is valid. */
                            MESOSPHERE_INIT_ABORT_UNLESS(l3_entry[i].IsCompatibleWithAttribute(attr_before, true));
                            static_cast<PageTableEntry *>(l3_entry)[i] = InvalidPageTableEntry;
                        }
                        cpu::DataSynchronizationBarrierInnerShareable();
                        cpu::InvalidateEntireTlb();

                        /* Create a new contiguous L3 block. */
                        for (size_t i = 0; i < L3ContiguousBlockSize / L3BlockSize; i++) {
                            l3_entry[i] = L3PageTableEntry(block + L3BlockSize * i, attr_after, true);
                        }

                        virt_addr += L3ContiguousBlockSize;
                        size      -= L3ContiguousBlockSize;
                    } else {
                        /* Ensure that we are allowed to have an L3 block here. */
                        MESOSPHERE_INIT_ABORT_UNLESS(util::IsAligned(GetInteger(virt_addr), L3BlockSize));
                        MESOSPHERE_INIT_ABORT_UNLESS(util::IsAligned(GetInteger(block),     L3BlockSize));
                        MESOSPHERE_INIT_ABORT_UNLESS(util::IsAligned(size,                  L3BlockSize));
                        MESOSPHERE_INIT_ABORT_UNLESS(l3_entry->IsCompatibleWithAttribute(attr_before, false));

                        /* Invalidate the existing L3 block. */
                        *static_cast<PageTableEntry *>(l3_entry) = InvalidPageTableEntry;
                        cpu::DataSynchronizationBarrierInnerShareable();
                        cpu::InvalidateEntireTlb();

                        /* Create new L3 block. */
                        *l3_entry = L3PageTableEntry(block, attr_after, false);

                        virt_addr += L3BlockSize;
                        size      -= L3BlockSize;
                    }
                }

                /* Ensure data consistency after we complete reprotection. */
                cpu::DataSynchronizationBarrierInnerShareable();
            }

    };

    class KInitialPageAllocator : public KInitialPageTable::IPageAllocator {
        private:
            uintptr_t next_address;
        public:
            constexpr ALWAYS_INLINE KInitialPageAllocator() : next_address(Null<uintptr_t>) { /* ... */ }

            ALWAYS_INLINE void Initialize(uintptr_t address) {
                this->next_address = address;
            }

            ALWAYS_INLINE uintptr_t GetFinalNextAddress() {
                const uintptr_t final_address = this->next_address;
                this->next_address = Null<uintptr_t>;
                return final_address;
            }

            ALWAYS_INLINE uintptr_t GetFinalState() {
                return this->GetFinalNextAddress();
            }
        public:
            virtual KPhysicalAddress Allocate() override {
                MESOSPHERE_INIT_ABORT_UNLESS(this->next_address != Null<uintptr_t>);
                const uintptr_t allocated = this->next_address;
                this->next_address += PageSize;
                std::memset(reinterpret_cast<void *>(allocated), 0, PageSize);
                return allocated;
            }

            /* No need to override free. The default does nothing, and so would we. */
    };

}
