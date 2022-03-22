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
#include <stratosphere.hpp>
#include "htc_tenv_allocator.hpp"

namespace ams::htc::tenv::impl {

    namespace {

        constinit AllocateFunction g_allocate     = nullptr;
        constinit DeallocateFunction g_deallocate = nullptr;

    }

    void InitializeAllocator(AllocateFunction allocate, DeallocateFunction deallocate) {
        /* Check that we don't already have allocator functions. */
        AMS_ASSERT(g_allocate == nullptr);
        AMS_ASSERT(g_deallocate == nullptr);

        /* Set our allocator functions. */
        g_allocate = allocate;
        g_deallocate = deallocate;

        /* Check that we have allocator functions. */
        AMS_ASSERT(g_allocate != nullptr);
        AMS_ASSERT(g_deallocate != nullptr);
    }

    void *Allocate(size_t size) {
        /* Check that we have an allocator. */
        AMS_ASSERT(g_allocate != nullptr);

        return g_allocate(size);
    }

    void Deallocate(void *p, size_t size) {
        /* Check that we have a deallocator. */
        AMS_ASSERT(g_deallocate != nullptr);

        return g_deallocate(p, size);
    }

}
