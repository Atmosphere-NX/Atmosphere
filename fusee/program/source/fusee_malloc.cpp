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
#include "fusee_malloc.hpp"
#include "fusee_fatal.hpp"

namespace ams::nxboot {

    namespace {

        constinit uintptr_t g_heap_address = 0xC1000000;

    }

    void *AllocateMemory(size_t size) {
        /* Get the current heap address. */
        void * const allocated = reinterpret_cast<void *>(g_heap_address);

        /* Advance the current heap address. */
        g_heap_address += size;

        /* Return the allocated chunk. */
        return allocated;
    }

}
