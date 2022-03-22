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
#include "fatal_service.hpp"

namespace ams::fatal::srv {

    class ITask {
        protected:
            const ThrowContext *m_context = nullptr;
        public:
            void Initialize(const ThrowContext *context) {
                m_context = context;
            }

            virtual Result Run() = 0;
            virtual const char *GetName() const = 0;
            virtual u8 *GetStack() = 0;
            virtual size_t GetStackSize() const = 0;
    };

    template<size_t _StackSize>
    class ITaskWithStack : public ITask {
        public:
            static constexpr size_t StackSize = _StackSize;
            static_assert(util::IsAligned(StackSize, os::MemoryPageSize), "StackSize alignment");
        protected:
            alignas(os::MemoryPageSize) u8 m_stack_mem[StackSize] = {};
        public:
            virtual u8 *GetStack() override final {
                return m_stack_mem;
            }

            virtual size_t GetStackSize() const override final {
                return StackSize;
            }
    };

    using ITaskWithDefaultStack = ITaskWithStack<0x2000>;

    void RunTasks(const ThrowContext *ctx);

}