/*
 * Copyright (c) Atmosphère-NX
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
#include <mesosphere/kern_common.hpp>

#if defined(ATMOSPHERE_ARCH_ARM64)

    #include <mesosphere/arch/arm64/kern_k_page_table.hpp>
    #include <mesosphere/arch/arm64/kern_k_supervisor_page_table.hpp>
    #include <mesosphere/arch/arm64/kern_k_process_page_table.hpp>
    namespace ams::kern {
        using ams::kern::arch::arm64::KPageTable;
        using ams::kern::arch::arm64::KSupervisorPageTable;
        using ams::kern::arch::arm64::KProcessPageTable;
    }

#else

    #error "Unknown architecture for KPageTable"

#endif

namespace ams::kern {

    /* NOTE: These three functions (Operate, Operate, FinalizeUpdate) are virtual functions */
    /* in Nintendo's kernel. We devirtualize them, since KPageTable is the only derived */
    /* class, and this avoids unnecessary virtual function calls. */
    static_assert(std::derived_from<KPageTable, KPageTableBase>);

    ALWAYS_INLINE Result KPageTableBase::Operate(PageLinkedList *page_list, KProcessAddress virt_addr, size_t num_pages, KPhysicalAddress phys_addr, bool is_pa_valid, const KPageProperties properties, OperationType operation, bool reuse_ll) {
        return static_cast<KPageTable *>(this)->OperateImpl(page_list, virt_addr, num_pages, phys_addr, is_pa_valid, properties, operation, reuse_ll);
    }

    ALWAYS_INLINE Result KPageTableBase::Operate(PageLinkedList *page_list, KProcessAddress virt_addr, size_t num_pages, const KPageGroup &page_group, const KPageProperties properties, OperationType operation, bool reuse_ll) {
        return static_cast<KPageTable *>(this)->OperateImpl(page_list, virt_addr, num_pages, page_group, properties, operation, reuse_ll);
    }

    ALWAYS_INLINE void KPageTableBase::FinalizeUpdate(PageLinkedList *page_list) {
        return static_cast<KPageTable *>(this)->FinalizeUpdateImpl(page_list);
    }

}