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
#include <mesosphere/kern_slab_helpers.hpp>
#include <mesosphere/kern_k_memory_layout.hpp>

namespace ams::kern {

    class KPageBuffer : public KSlabAllocated<KPageBuffer> {
        private:
            alignas(PageSize) u8 m_buffer[PageSize];
        public:
            KPageBuffer() {
                std::memset(m_buffer, 0, sizeof(m_buffer));
            }

            ALWAYS_INLINE KPhysicalAddress GetPhysicalAddress() const {
                return KMemoryLayout::GetLinearPhysicalAddress(KVirtualAddress(this));
            }

            static ALWAYS_INLINE KPageBuffer *FromPhysicalAddress(KPhysicalAddress phys_addr) {
                const KVirtualAddress virt_addr = KMemoryLayout::GetLinearVirtualAddress(phys_addr);

                MESOSPHERE_ASSERT(util::IsAligned(GetInteger(phys_addr), PageSize));
                MESOSPHERE_ASSERT(util::IsAligned(GetInteger(virt_addr), PageSize));

                return GetPointer<KPageBuffer>(virt_addr);
            }
    };
    static_assert(sizeof(KPageBuffer)  == PageSize);
    static_assert(alignof(KPageBuffer) == PageSize);

}
