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

#if defined(ATMOSPHERE_OS_HORIZON)
    #include <stratosphere/diag/impl/diag_backtrace_impl.os.horizon.hpp>
#elif defined(ATMOSPHERE_OS_WINDOWS)
    #include <stratosphere/diag/impl/diag_backtrace_impl.os.windows.hpp>
#elif defined(ATMOSPHERE_OS_LINUX)
    #include <stratosphere/diag/impl/diag_backtrace_impl.os.linux.hpp>
#elif defined(ATMOSPHERE_OS_MACOS)
    #include <stratosphere/diag/impl/diag_backtrace_impl.os.macos.hpp>
#else
    #error "Unknown OS for diag::Backtrace"
#endif

namespace ams::diag {

    size_t GetBacktrace(uintptr_t *out, size_t out_size);

    #if defined(ATMOSPHERE_OS_HORIZON)
    size_t GetBacktace(uintptr_t *out, size_t out_size, uintptr_t fp, uintptr_t sp, uintptr_t pc);
    #endif

    class Backtrace {
        private:
            impl::Backtrace m_impl;
        public:
            NOINLINE Backtrace() {
                m_impl.Initialize();
                m_impl.Step();
            }

            #if defined(ATMOSPHERE_OS_HORIZON)
            Backtrace(uintptr_t fp, uintptr_t sp, uintptr_t pc) {
                m_impl.Initialize(fp, sp, pc);
            }
            #endif

            bool Step() { return m_impl.Step(); }

            uintptr_t GetStackPointer()  const { return m_impl.GetStackPointer(); }
            uintptr_t GetReturnAddress() const { return m_impl.GetReturnAddress(); }
    };

}
