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
#include <stratosphere.hpp>

namespace ams::erpt::srv {

    class RefCount {
        private:
            static constexpr u32 MaxReferenceCount = 1000;
            std::atomic<u32> m_ref_count;
        public:
            RefCount() : m_ref_count(0) { /* ... */ }

            void AddReference() {
                const auto prev = m_ref_count.fetch_add(1);
                AMS_ABORT_UNLESS(prev <= MaxReferenceCount);
            }

            bool RemoveReference() {
                auto prev = m_ref_count.fetch_sub(1);
                AMS_ABORT_UNLESS(prev != 0);
                return prev == 1;
            }
    };

}
