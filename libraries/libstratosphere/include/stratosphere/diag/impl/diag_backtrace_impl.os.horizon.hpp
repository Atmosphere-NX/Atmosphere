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
            static constexpr size_t MemoryInfoBufferSize = 5;
        public:
            struct StackInfo {
                uintptr_t stack_top;
                uintptr_t stack_bottom;
            };
        private:
            s64 m_memory_info_buffer[MemoryInfoBufferSize]{};
            StackInfo *m_current_stack_info = nullptr;
            StackInfo m_exception_stack_info{};
            StackInfo m_normal_stack_info{};
            uintptr_t m_fp = 0;
            uintptr_t m_prev_fp = 0;
            uintptr_t m_lr = 0;
        public:
            Backtrace() = default;

            void Initialize();
            void Initialize(uintptr_t fp, uintptr_t sp, uintptr_t pc);

            bool Step();
            uintptr_t GetStackPointer() const;
            uintptr_t GetReturnAddress() const;
        private:
            void SetStackInfo(uintptr_t fp, uintptr_t sp);
    };

}
