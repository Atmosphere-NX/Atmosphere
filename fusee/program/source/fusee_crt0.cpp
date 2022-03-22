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
#include <exosphere.hpp>
#include "fusee_exception_handler.hpp"

extern "C" void __libc_init_array();

namespace ams::nxboot::crt0 {

    namespace {

        ALWAYS_INLINE void SetExceptionVector(u32 which, uintptr_t impl) {
            reg::Write(secmon::MemoryRegionPhysicalDeviceExceptionVectors.GetAddress() + 0x200 + sizeof(u32) * which, static_cast<u32>(impl));
        }

    }

    void Initialize() {
        /* TODO: Collect timing information? */

        /* Setup exception vectors. */
        {
            SetExceptionVector(0, reinterpret_cast<uintptr_t>(::ams::nxboot::ExceptionHandler0));
            SetExceptionVector(1, reinterpret_cast<uintptr_t>(::ams::nxboot::ExceptionHandler1));
            SetExceptionVector(2, reinterpret_cast<uintptr_t>(::ams::nxboot::ExceptionHandler2));
            SetExceptionVector(3, reinterpret_cast<uintptr_t>(::ams::nxboot::ExceptionHandler3));
            SetExceptionVector(4, reinterpret_cast<uintptr_t>(::ams::nxboot::ExceptionHandler4));
            SetExceptionVector(5, reinterpret_cast<uintptr_t>(::ams::nxboot::ExceptionHandler5));
            SetExceptionVector(6, reinterpret_cast<uintptr_t>(::ams::nxboot::ExceptionHandler6));
            SetExceptionVector(7, reinterpret_cast<uintptr_t>(::ams::nxboot::ExceptionHandler7));
        }

        /* Call init array. */
        __libc_init_array();
    }

}
