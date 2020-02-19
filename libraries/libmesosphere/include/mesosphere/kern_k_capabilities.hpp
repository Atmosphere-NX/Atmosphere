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
#include <mesosphere/kern_common.hpp>
#include <mesosphere/kern_select_page_table.hpp>
#include <mesosphere/kern_svc.hpp>

namespace ams::kern {

    class KCapabilities {
        private:
            static constexpr size_t SvcFlagCount = svc::NumSupervisorCalls / BITSIZEOF(u8);
            static constexpr size_t IrqFlagCount = /* TODO */0x80;
        private:
            u8 svc_access_flags[SvcFlagCount]{};
            u8 irq_access_flags[IrqFlagCount]{};
            u64 core_mask{};
            u64 priority_mask{};
            util::BitPack32 debug_capabilities;
            s32 handle_table_size{};
            util::BitPack32 intended_kernel_version;
            u32 program_type{};
        public:
            constexpr KCapabilities() : debug_capabilities(0), intended_kernel_version(0) { /* ... */ }

            Result Initialize(const u32 *caps, s32 num_caps, KProcessPageTable *page_table);

            constexpr u64 GetCoreMask() const { return this->core_mask; }
            constexpr u64 GetPriorityMask() const { return this->priority_mask; }

            /* TODO: Member functions. */
    };

}
