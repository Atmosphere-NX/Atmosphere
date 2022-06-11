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

namespace ams::os::impl {

    class MemoryHeapManagerHorizonImpl {
        public:
            Result SetHeapSize(uintptr_t *out, size_t size) {
                R_TRY_CATCH(svc::SetHeapSize(out, size)) {
                    R_CONVERT(svc::ResultOutOfMemory,   os::ResultOutOfMemory())
                    R_CONVERT(svc::ResultLimitReached,  os::ResultOutOfMemory())
                    R_CONVERT(svc::ResultOutOfResource, os::ResultOutOfMemory())
                } R_END_TRY_CATCH;

                R_SUCCEED();
            }
    };

    using MemoryHeapManagerImpl = MemoryHeapManagerHorizonImpl;

}
