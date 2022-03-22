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

namespace ams::init {

    namespace {

        constinit void *g_malloc_region_address = nullptr;
        constinit size_t g_malloc_region_size   = 0;

        constinit util::TypedStorage<mem::StandardAllocator> g_malloc_allocator;

    }

    void InitializeAllocator(void *address, size_t size, bool cache_enabled) {
        /* Check pre-conditions. */
        AMS_ABORT_UNLESS(g_malloc_region_size == 0);
        AMS_ABORT_UNLESS(size > 0);

        /* Construct malloc allocator. */
        util::ConstructAt(g_malloc_allocator);

        /* Initialize allocator. */
        util::GetReference(g_malloc_allocator).Initialize(address, size, cache_enabled);

        /* Set malloc globals. */
        g_malloc_region_address = address;
        g_malloc_region_size    = size;
    }

    void InitializeAllocator(void *address, size_t size) {
        return InitializeAllocator(address, size, false);
    }

    mem::StandardAllocator *GetAllocator() {
        /* Check pre-conditions. */
        AMS_ASSERT(g_malloc_region_size > 0);

        return util::GetPointer(g_malloc_allocator);
    }

}

extern "C" void *malloc(size_t size) {
    /* We require that an allocator region exists. */
    if (::ams::init::g_malloc_region_size == 0) {
        return nullptr;
    }

    /* Try to allocate. */
    void *ptr = ::ams::util::GetReference(::ams::init::g_malloc_allocator).Allocate(size);
    if (ptr == nullptr) {
        errno = ENOMEM;
    }

    return ptr;
}

extern "C" void free(void *ptr) {
    /* We require that an allocator region exists. */
    if (::ams::init::g_malloc_region_size == 0) {
        return;
    }

    if (ptr != nullptr) {
        ::ams::util::GetReference(::ams::init::g_malloc_allocator).Free(ptr);
    }
}

extern "C" void *calloc(size_t num, size_t size) {
    /* We require that an allocator region exists. */
    if (::ams::init::g_malloc_region_size == 0) {
        return nullptr;
    }

    /* Allocate the total needed space. */
    const size_t total = num * size;
    void *ptr = std::malloc(total);

    /* Zero the memory if needed. */
    if (ptr != nullptr) {
        std::memset(ptr, 0, total);
    } else {
        errno = ENOMEM;
    }

    return ptr;
}

extern "C" void *realloc(void *ptr, size_t new_size) {
    /* We require that an allocator region exists. */
    if (::ams::init::g_malloc_region_size == 0) {
        return nullptr;
    }

    /* Try to reallocate. */
    void *r = ::ams::util::GetReference(::ams::init::g_malloc_allocator).Reallocate(ptr, new_size);
    if (r == nullptr) {
        errno = ENOMEM;
    }

    return r;
}

extern "C" void *aligned_alloc(size_t align, size_t size) {
    /* We require that an allocator region exists. */
    if (::ams::init::g_malloc_region_size == 0) {
        return nullptr;
    }

    /* Try to allocate. */
    void *ptr = ::ams::util::GetReference(::ams::init::g_malloc_allocator).Allocate(size, align);
    if (ptr == nullptr) {
        errno = ENOMEM;
    }

    return ptr;
}

extern "C" size_t malloc_usable_size(void *ptr) {
    /* We require that an allocator region exists. */
    if (::ams::init::g_malloc_region_size == 0) {
        return 0;
    }

    /* Try to get the usable size. */
    if (ptr == nullptr) {
        errno = ENOMEM;
        return 0;
    }

    return ::ams::util::GetReference(::ams::init::g_malloc_allocator).GetSizeOf(ptr);
}
