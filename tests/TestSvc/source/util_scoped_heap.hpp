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
#include "util_test_framework.hpp"

namespace ams::test {

    class ScopedHeap {
        NON_COPYABLE(ScopedHeap);
        NON_MOVEABLE(ScopedHeap);
        private:
            uintptr_t m_address;
            size_t m_size;
        public:
            explicit ScopedHeap(size_t size) {
                this->SetHeapSize(size);
            }

            ~ScopedHeap() {
                const auto result = svc::SetHeapSize(std::addressof(m_address), 0);
                DOCTEST_CHECK(R_SUCCEEDED(result));
            }

            void SetHeapSize(size_t size) {
                m_size = util::AlignUp(size, svc::HeapSizeAlignment);

                const auto result = svc::SetHeapSize(std::addressof(m_address), m_size);
                DOCTEST_CHECK(R_SUCCEEDED(result));
            }

            uintptr_t GetAddress() const { return m_address; }
            size_t GetSize() const { return m_size; }
    };

}