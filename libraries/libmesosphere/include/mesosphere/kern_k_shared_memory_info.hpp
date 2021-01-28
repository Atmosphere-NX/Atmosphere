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
#include <mesosphere/kern_slab_helpers.hpp>

namespace ams::kern {

    class KSharedMemory;

    class KSharedMemoryInfo : public KSlabAllocated<KSharedMemoryInfo>, public util::IntrusiveListBaseNode<KSharedMemoryInfo> {
        private:
            KSharedMemory *m_shared_memory;
            size_t m_reference_count;
        public:
            constexpr KSharedMemoryInfo() : m_shared_memory(), m_reference_count() { /* ... */ }
            ~KSharedMemoryInfo() { /* ... */ }

            constexpr void Initialize(KSharedMemory *m) {
                MESOSPHERE_ASSERT_THIS();
                m_shared_memory   = m;
                m_reference_count = 0;
            }

            constexpr void Open() {
                ++m_reference_count;
                MESOSPHERE_ASSERT(m_reference_count > 0);
            }

            constexpr bool Close() {
                MESOSPHERE_ASSERT(m_reference_count > 0);
                return (--m_reference_count) == 0;
            }

            constexpr KSharedMemory *GetSharedMemory() const { return m_shared_memory; }
            constexpr size_t GetReferenceCount() const { return m_reference_count; }
    };

}
