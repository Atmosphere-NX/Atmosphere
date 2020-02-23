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

#include "defines.hpp"
#include "hvisor_core_context.hpp"

namespace ams::hvisor {

    class FpuRegisterCache final {
        SINGLETON_WITH_ATTRS(FpuRegisterCache, TEMPORARY);
        public:
            struct Storage {
                u128 q[32];
                u64 fpsr;
                u64 fpcr;
            };
            static_assert(std::is_standard_layout_v<Storage>);

        private:
            static void ReloadRegisters(const Storage *storage);
            static void DumpRegisters(Storage *storage);

        private:
            Storage m_storage{};
            u32 m_coreId = 0;
            bool m_valid = false;
            bool m_dirty = false;

        public:
            constexpr void TakeOwnership()
            {
                if (m_coreId != currentCoreCtx->GetCoreId()) {
                    m_valid = false;
                    m_dirty = false;
                }
                m_coreId = currentCoreCtx->GetCoreId();
            }

            Storage &GetStorageRef()
            {
                return m_storage;
            }

            Storage &ReadRegisters()
            {
                if (!m_valid) {
                    DumpRegisters(&m_storage);
                    m_valid = true;
                }
                return m_storage;
            }

            constexpr void CommitRegisters()
            {
                m_dirty = true;
                // Because the caller rewrote the entire cache in the event it didn't read it before:
                m_valid = true;
            }

            void CleanInvalidate()
            {
                if (m_dirty && m_coreId == currentCoreCtx->GetCoreId()) {
                    ReloadRegisters(&m_storage);
                    m_dirty = false;
                }
                m_valid = false;
            }
        public:
            constexpr FpuRegisterCache() = default;
    };

}
