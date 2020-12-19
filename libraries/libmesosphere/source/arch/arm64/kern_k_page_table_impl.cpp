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
#include <mesosphere.hpp>

namespace ams::kern::arch::arm64 {

    void KPageTableImpl::InitializeForKernel(void *tb, KVirtualAddress start, KVirtualAddress end) {
        m_table       = static_cast<L1PageTableEntry *>(tb);
        m_is_kernel   = true;
        m_num_entries = util::AlignUp(end - start, L1BlockSize) / L1BlockSize;
    }

    void KPageTableImpl::InitializeForProcess(void *tb, KVirtualAddress start, KVirtualAddress end) {
        m_table       = static_cast<L1PageTableEntry *>(tb);
        m_is_kernel   = false;
        m_num_entries = util::AlignUp(end - start, L1BlockSize) / L1BlockSize;
    }

    L1PageTableEntry *KPageTableImpl::Finalize() {
        return m_table;
    }

    bool KPageTableImpl::ExtractL3Entry(TraversalEntry *out_entry, TraversalContext *out_context, const L3PageTableEntry *l3_entry, KProcessAddress virt_addr) const {
        /* Set the L3 entry. */
        out_context->l3_entry = l3_entry;

        if (l3_entry->IsBlock()) {
            /* Set the output entry. */
            out_entry->phys_addr = l3_entry->GetBlock() + (virt_addr & (L3BlockSize - 1));
            if (l3_entry->IsContiguous()) {
                out_entry->block_size = L3ContiguousBlockSize;
            } else {
                out_entry->block_size = L3BlockSize;
            }
            out_entry->sw_reserved_bits = l3_entry->GetSoftwareReservedBits();

            return true;
        } else {
            out_entry->phys_addr        = Null<KPhysicalAddress>;
            out_entry->block_size       = L3BlockSize;
            out_entry->sw_reserved_bits = 0;
            return false;
        }
    }

    bool KPageTableImpl::ExtractL2Entry(TraversalEntry *out_entry, TraversalContext *out_context, const L2PageTableEntry *l2_entry, KProcessAddress virt_addr) const {
        /* Set the L2 entry. */
        out_context->l2_entry = l2_entry;

        if (l2_entry->IsBlock()) {
            /* Set the output entry. */
            out_entry->phys_addr = l2_entry->GetBlock() + (virt_addr & (L2BlockSize - 1));
            if (l2_entry->IsContiguous()) {
                out_entry->block_size = L2ContiguousBlockSize;
            } else {
                out_entry->block_size = L2BlockSize;
            }
            out_entry->sw_reserved_bits = l2_entry->GetSoftwareReservedBits();

            /* Set the output context. */
            out_context->l3_entry = nullptr;
            return true;
        } else if (l2_entry->IsTable()) {
            return this->ExtractL3Entry(out_entry, out_context, this->GetL3EntryFromTable(GetPageTableVirtualAddress(l2_entry->GetTable()), virt_addr), virt_addr);
        } else {
            out_entry->phys_addr        = Null<KPhysicalAddress>;
            out_entry->block_size       = L2BlockSize;
            out_entry->sw_reserved_bits = 0;
            out_context->l3_entry = nullptr;
            return false;
        }
    }

    bool KPageTableImpl::ExtractL1Entry(TraversalEntry *out_entry, TraversalContext *out_context, const L1PageTableEntry *l1_entry, KProcessAddress virt_addr) const {
        /* Set the L1 entry. */
        out_context->l1_entry = l1_entry;

        if (l1_entry->IsBlock()) {
            /* Set the output entry. */
            out_entry->phys_addr = l1_entry->GetBlock() + (virt_addr & (L1BlockSize - 1));
            if (l1_entry->IsContiguous()) {
                out_entry->block_size = L1ContiguousBlockSize;
            } else {
                out_entry->block_size = L1BlockSize;
            }
            out_entry->sw_reserved_bits = l1_entry->GetSoftwareReservedBits();

            /* Set the output context. */
            out_context->l2_entry = nullptr;
            out_context->l3_entry = nullptr;
            return true;
        } else if (l1_entry->IsTable()) {
            return this->ExtractL2Entry(out_entry, out_context, this->GetL2EntryFromTable(GetPageTableVirtualAddress(l1_entry->GetTable()), virt_addr), virt_addr);
        } else {
            out_entry->phys_addr        = Null<KPhysicalAddress>;
            out_entry->block_size       = L1BlockSize;
            out_entry->sw_reserved_bits = 0;
            out_context->l2_entry = nullptr;
            out_context->l3_entry = nullptr;
            return false;
        }
    }

    bool KPageTableImpl::BeginTraversal(TraversalEntry *out_entry, TraversalContext *out_context, KProcessAddress address) const {
        /* Setup invalid defaults. */
        out_entry->phys_addr        = Null<KPhysicalAddress>;
        out_entry->block_size       = L1BlockSize;
        out_entry->sw_reserved_bits = 0;
        out_context->l1_entry = m_table + m_num_entries;
        out_context->l2_entry = nullptr;
        out_context->l3_entry = nullptr;

        /* Validate that we can read the actual entry. */
        const size_t l0_index = GetL0Index(address);
        const size_t l1_index = GetL1Index(address);
        if (m_is_kernel) {
            /* Kernel entries must be accessed via TTBR1. */
            if ((l0_index != MaxPageTableEntries - 1) || (l1_index < MaxPageTableEntries - m_num_entries)) {
                return false;
            }
        } else {
            /* User entries must be accessed with TTBR0. */
            if ((l0_index != 0) || l1_index >= m_num_entries) {
                return false;
            }
        }

        /* Extract the entry. */
        const bool valid = this->ExtractL1Entry(out_entry, out_context, this->GetL1Entry(address), address);

        /* Update the context for next traversal. */
        switch (out_entry->block_size) {
            case L1ContiguousBlockSize:
                out_context->l1_entry += (L1ContiguousBlockSize / L1BlockSize) - GetContiguousL1Offset(address) / L1BlockSize;
                break;
            case L1BlockSize:
                out_context->l1_entry += 1;
                break;
            case L2ContiguousBlockSize:
                out_context->l1_entry += 1;
                out_context->l2_entry += (L2ContiguousBlockSize / L2BlockSize) - GetContiguousL2Offset(address) / L2BlockSize;
                break;
            case L2BlockSize:
                out_context->l1_entry += 1;
                out_context->l2_entry += 1;
                break;
            case L3ContiguousBlockSize:
                out_context->l1_entry += 1;
                out_context->l2_entry += 1;
                out_context->l3_entry += (L3ContiguousBlockSize / L3BlockSize) - GetContiguousL3Offset(address) / L3BlockSize;
                break;
            case L3BlockSize:
                out_context->l1_entry += 1;
                out_context->l2_entry += 1;
                out_context->l3_entry += 1;
                break;
            MESOSPHERE_UNREACHABLE_DEFAULT_CASE();
        }

        return valid;
    }

    bool KPageTableImpl::ContinueTraversal(TraversalEntry *out_entry, TraversalContext *context) const {
        bool valid = false;

        /* Check if we're not at the end of an L3 table. */
        if (!util::IsAligned(reinterpret_cast<uintptr_t>(context->l3_entry), PageSize)) {
            valid = this->ExtractL3Entry(out_entry, context, context->l3_entry, Null<KProcessAddress>);

            switch (out_entry->block_size) {
                case L3ContiguousBlockSize:
                    context->l3_entry += (L3ContiguousBlockSize / L3BlockSize);
                    break;
                case L3BlockSize:
                    context->l3_entry += 1;
                    break;
                MESOSPHERE_UNREACHABLE_DEFAULT_CASE();
            }
        } else if (!util::IsAligned(reinterpret_cast<uintptr_t>(context->l2_entry), PageSize)) {
            /* We're not at the end of an L2 table. */
            valid = this->ExtractL2Entry(out_entry, context, context->l2_entry, Null<KProcessAddress>);

            switch (out_entry->block_size) {
                case L2ContiguousBlockSize:
                    context->l2_entry += (L2ContiguousBlockSize / L2BlockSize);
                    break;
                case L2BlockSize:
                    context->l2_entry += 1;
                    break;
                case L3ContiguousBlockSize:
                    context->l2_entry += 1;
                    context->l3_entry += (L3ContiguousBlockSize / L3BlockSize);
                    break;
                case L3BlockSize:
                    context->l2_entry += 1;
                    context->l3_entry += 1;
                    break;
                MESOSPHERE_UNREACHABLE_DEFAULT_CASE();
            }
        } else {
            /* We need to update the l1 entry. */
            const size_t l1_index = context->l1_entry - m_table;
            if (l1_index < m_num_entries) {
                valid = this->ExtractL1Entry(out_entry, context, context->l1_entry, Null<KProcessAddress>);
            } else {
                /* Invalid, end traversal. */
                out_entry->phys_addr        = Null<KPhysicalAddress>;
                out_entry->block_size       = L1BlockSize;
                out_entry->sw_reserved_bits = 0;
                context->l1_entry = m_table + m_num_entries;
                context->l2_entry = nullptr;
                context->l3_entry = nullptr;
                return false;
            }

            switch (out_entry->block_size) {
                case L1ContiguousBlockSize:
                    context->l1_entry += (L1ContiguousBlockSize / L1BlockSize);
                    break;
                case L1BlockSize:
                    context->l1_entry += 1;
                    break;
                case L2ContiguousBlockSize:
                    context->l1_entry += 1;
                    context->l2_entry += (L2ContiguousBlockSize / L2BlockSize);
                    break;
                case L2BlockSize:
                    context->l1_entry += 1;
                    context->l2_entry += 1;
                    break;
                case L3ContiguousBlockSize:
                    context->l1_entry += 1;
                    context->l2_entry += 1;
                    context->l3_entry += (L3ContiguousBlockSize / L3BlockSize);
                    break;
                case L3BlockSize:
                    context->l1_entry += 1;
                    context->l2_entry += 1;
                    context->l3_entry += 1;
                    break;
                MESOSPHERE_UNREACHABLE_DEFAULT_CASE();
            }
        }

        return valid;
    }

    bool KPageTableImpl::GetPhysicalAddress(KPhysicalAddress *out, KProcessAddress address) const {
        /* Validate that we can read the actual entry. */
        const size_t l0_index = GetL0Index(address);
        const size_t l1_index = GetL1Index(address);
        if (m_is_kernel) {
            /* Kernel entries must be accessed via TTBR1. */
            if ((l0_index != MaxPageTableEntries - 1) || (l1_index < MaxPageTableEntries - m_num_entries)) {
                return false;
            }
        } else {
            /* User entries must be accessed with TTBR0. */
            if ((l0_index != 0) || l1_index >= m_num_entries) {
                return false;
            }
        }

        /* Try to get from l1 table. */
        const L1PageTableEntry *l1_entry = this->GetL1Entry(address);
        if (l1_entry->IsBlock()) {
            *out = l1_entry->GetBlock() + GetL1Offset(address);
            return true;
        } else if (!l1_entry->IsTable()) {
            return false;
        }

        /* Try to get from l2 table. */
        const L2PageTableEntry *l2_entry = this->GetL2Entry(l1_entry, address);
        if (l2_entry->IsBlock()) {
            *out = l2_entry->GetBlock() + GetL2Offset(address);
            return true;
        } else if (!l2_entry->IsTable()) {
            return false;
        }

        /* Try to get from l3 table. */
        const L3PageTableEntry *l3_entry = this->GetL3Entry(l2_entry, address);
        if (l3_entry->IsBlock()) {
            *out = l3_entry->GetBlock() + GetL3Offset(address);
            return true;
        }

        return false;
    }

    void KPageTableImpl::Dump(uintptr_t start, size_t size) const {
        /* If zero size, there's nothing to dump. */
        if (size == 0) {
            return;
        }

        /* Define extents. */
        const uintptr_t end  = start + size;
        const uintptr_t last = end - 1;

        /* Define tracking variables. */
        bool unmapped = false;
        uintptr_t unmapped_start = 0;

        /* Walk the table. */
        uintptr_t cur = start;
        while (cur < end) {
            /* Validate that we can read the actual entry. */
            const size_t l0_index = GetL0Index(cur);
            const size_t l1_index = GetL1Index(cur);
            if (m_is_kernel) {
                /* Kernel entries must be accessed via TTBR1. */
                if ((l0_index != MaxPageTableEntries - 1) || (l1_index < MaxPageTableEntries - m_num_entries)) {
                    return;
                }
            } else {
                /* User entries must be accessed with TTBR0. */
                if ((l0_index != 0) || l1_index >= m_num_entries) {
                    return;
                }
            }

            /* Try to get from l1 table. */
            const L1PageTableEntry *l1_entry = this->GetL1Entry(cur);
            if (l1_entry->IsBlock()) {
                /* Update. */
                cur = util::AlignDown(cur, L1BlockSize);
                if (unmapped) {
                    unmapped = false;
                    MESOSPHERE_RELEASE_LOG("%016lx - %016lx: not mapped\n", unmapped_start, cur - 1);
                }

                /* Print. */
                MESOSPHERE_RELEASE_LOG("%016lx: %016lx PA=%p SZ=1G Mapped=%d UXN=%d PXN=%d Cont=%d nG=%d AF=%d SH=%x RO=%d UA=%d NS=%d AttrIndx=%d NoMerge=%d,%d,%d\n", cur,
                                                                                                                                                                        *reinterpret_cast<const u64 *>(l1_entry),
                                                                                                                                                                        reinterpret_cast<void *>(GetInteger(l1_entry->GetBlock())),
                                                                                                                                                                        l1_entry->IsMapped(),
                                                                                                                                                                        l1_entry->IsUserExecuteNever(),
                                                                                                                                                                        l1_entry->IsPrivilegedExecuteNever(),
                                                                                                                                                                        l1_entry->IsContiguous(),
                                                                                                                                                                        !l1_entry->IsGlobal(),
                                                                                                                                                                        static_cast<int>(l1_entry->GetAccessFlag()),
                                                                                                                                                                        static_cast<unsigned int>(l1_entry->GetShareable()),
                                                                                                                                                                        l1_entry->IsReadOnly(),
                                                                                                                                                                        l1_entry->IsUserAccessible(),
                                                                                                                                                                        l1_entry->IsNonSecure(),
                                                                                                                                                                        static_cast<int>(l1_entry->GetPageAttribute()),
                                                                                                                                                                        l1_entry->IsHeadMergeDisabled(),
                                                                                                                                                                        l1_entry->IsHeadAndBodyMergeDisabled(),
                                                                                                                                                                        l1_entry->IsTailMergeDisabled());

                /* Advance. */
                cur += L1BlockSize;
                continue;
            } else if (!l1_entry->IsTable()) {
                /* Update. */
                cur = util::AlignDown(cur, L1BlockSize);
                if (!unmapped) {
                    unmapped_start = cur;
                    unmapped       = true;
                }

                /* Advance. */
                cur += L1BlockSize;
                continue;
            }

            /* Try to get from l2 table. */
            const L2PageTableEntry *l2_entry = this->GetL2Entry(l1_entry, cur);
            if (l2_entry->IsBlock()) {
                /* Update. */
                cur = util::AlignDown(cur, L2BlockSize);
                if (unmapped) {
                    unmapped = false;
                    MESOSPHERE_RELEASE_LOG("%016lx - %016lx: not mapped\n", unmapped_start, cur - 1);
                }

                /* Print. */
                MESOSPHERE_RELEASE_LOG("%016lx: %016lx PA=%p SZ=2M Mapped=%d UXN=%d PXN=%d Cont=%d nG=%d AF=%d SH=%x RO=%d UA=%d NS=%d AttrIndx=%d NoMerge=%d,%d,%d\n", cur,
                                                                                                                                                                        *reinterpret_cast<const u64 *>(l2_entry),
                                                                                                                                                                        reinterpret_cast<void *>(GetInteger(l2_entry->GetBlock())),
                                                                                                                                                                        l2_entry->IsMapped(),
                                                                                                                                                                        l2_entry->IsUserExecuteNever(),
                                                                                                                                                                        l2_entry->IsPrivilegedExecuteNever(),
                                                                                                                                                                        l2_entry->IsContiguous(),
                                                                                                                                                                        !l2_entry->IsGlobal(),
                                                                                                                                                                        static_cast<int>(l2_entry->GetAccessFlag()),
                                                                                                                                                                        static_cast<unsigned int>(l2_entry->GetShareable()),
                                                                                                                                                                        l2_entry->IsReadOnly(),
                                                                                                                                                                        l2_entry->IsUserAccessible(),
                                                                                                                                                                        l2_entry->IsNonSecure(),
                                                                                                                                                                        static_cast<int>(l2_entry->GetPageAttribute()),
                                                                                                                                                                        l2_entry->IsHeadMergeDisabled(),
                                                                                                                                                                        l2_entry->IsHeadAndBodyMergeDisabled(),
                                                                                                                                                                        l2_entry->IsTailMergeDisabled());

                /* Advance. */
                cur += L2BlockSize;
                continue;
            } else if (!l2_entry->IsTable()) {
                /* Update. */
                cur = util::AlignDown(cur, L2BlockSize);
                if (!unmapped) {
                    unmapped_start = cur;
                    unmapped       = true;
                }

                /* Advance. */
                cur += L2BlockSize;
                continue;
            }

            /* Try to get from l3 table. */
            const L3PageTableEntry *l3_entry = this->GetL3Entry(l2_entry, cur);
            if (l3_entry->IsBlock()) {
                /* Update. */
                cur = util::AlignDown(cur, L3BlockSize);
                if (unmapped) {
                    unmapped = false;
                    MESOSPHERE_RELEASE_LOG("%016lx - %016lx: not mapped\n", unmapped_start, cur - 1);
                }

                /* Print. */
                MESOSPHERE_RELEASE_LOG("%016lx: %016lx PA=%p SZ=4K Mapped=%d UXN=%d PXN=%d Cont=%d nG=%d AF=%d SH=%x RO=%d UA=%d NS=%d AttrIndx=%d NoMerge=%d,%d,%d\n", cur,
                                                                                                                                                                        *reinterpret_cast<const u64 *>(l3_entry),
                                                                                                                                                                        reinterpret_cast<void *>(GetInteger(l3_entry->GetBlock())),
                                                                                                                                                                        l3_entry->IsMapped(),
                                                                                                                                                                        l3_entry->IsUserExecuteNever(),
                                                                                                                                                                        l3_entry->IsPrivilegedExecuteNever(),
                                                                                                                                                                        l3_entry->IsContiguous(),
                                                                                                                                                                        !l3_entry->IsGlobal(),
                                                                                                                                                                        static_cast<int>(l3_entry->GetAccessFlag()),
                                                                                                                                                                        static_cast<unsigned int>(l3_entry->GetShareable()),
                                                                                                                                                                        l3_entry->IsReadOnly(),
                                                                                                                                                                        l3_entry->IsUserAccessible(),
                                                                                                                                                                        l3_entry->IsNonSecure(),
                                                                                                                                                                        static_cast<int>(l3_entry->GetPageAttribute()),
                                                                                                                                                                        l3_entry->IsHeadMergeDisabled(),
                                                                                                                                                                        l3_entry->IsHeadAndBodyMergeDisabled(),
                                                                                                                                                                        l3_entry->IsTailMergeDisabled());

                /* Advance. */
                cur += L3BlockSize;
                continue;
            } else {
                /* Update. */
                cur = util::AlignDown(cur, L3BlockSize);
                if (!unmapped) {
                    unmapped_start = cur;
                    unmapped       = true;
                }

                /* Advance. */
                cur += L3BlockSize;
                continue;
            }
        }

        /* Print the last unmapped range if necessary. */
        if (unmapped) {
            MESOSPHERE_RELEASE_LOG("%016lx - %016lx: not mapped\n", unmapped_start, last);
        }
    }

    size_t KPageTableImpl::CountPageTables() const {
        size_t num_tables = 0;

        #if defined(MESOSPHERE_BUILD_FOR_DEBUGGING)
        {
            ++num_tables;
            for (size_t l1_index = 0; l1_index < m_num_entries; ++l1_index) {
                auto &l1_entry = m_table[l1_index];
                if (l1_entry.IsTable()) {
                    ++num_tables;
                    for (size_t l2_index = 0; l2_index < MaxPageTableEntries; ++l2_index) {
                        auto *l2_entry = GetPointer<L2PageTableEntry>(GetTableEntry(KMemoryLayout::GetLinearVirtualAddress(l1_entry.GetTable()), l2_index));
                        if (l2_entry->IsTable()) {
                            ++num_tables;
                        }
                    }
                }
            }
        }
        #endif

        return num_tables;
    }

}
