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

namespace ams::scs {

    namespace {

        alignas(os::MemoryPageSize) constinit u8 g_tenv_heap_storage[48_KB];
        constinit lmem::HeapHandle g_tenv_heap_handle = nullptr;
        constinit os::SdkMutex g_mutex;

        void InitializeExpHeap() {
            std::scoped_lock lk(g_mutex);
            g_tenv_heap_handle = lmem::CreateExpHeap(g_tenv_heap_storage, sizeof(g_tenv_heap_storage), lmem::CreateOption_None);
        }

        void *Allocate(size_t size) {
            std::scoped_lock lk(g_mutex);
            void *mem = lmem::AllocateFromExpHeap(g_tenv_heap_handle, size);
            return mem;
        }

        void Deallocate(void *p, size_t size) {
            AMS_UNUSED(size);

            std::scoped_lock lk(g_mutex);
            lmem::FreeToExpHeap(g_tenv_heap_handle, p);
        }

    }

    void InitializeTenvServiceManager() {
        /* Initialize the tenv heap. */
        InitializeExpHeap();

        /* Initialize the tenv library. */
        htc::tenv::Initialize(Allocate, Deallocate);
    }

}
