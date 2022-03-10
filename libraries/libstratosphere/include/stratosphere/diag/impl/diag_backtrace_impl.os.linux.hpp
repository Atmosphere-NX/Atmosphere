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
#include <vapours.hpp>

namespace ams::diag::impl {

    class Backtrace {
        private:
            static constexpr size_t BacktraceEntryCountMax = 0x80;
        private:
            void *m_backtrace_addresses[BacktraceEntryCountMax];
            size_t m_index = 0;
            size_t m_size = 0;
        public:
            Backtrace() = default;

            void Initialize();

            bool Step();
            uintptr_t GetStackPointer() const;
            uintptr_t GetReturnAddress() const;
    };

}
