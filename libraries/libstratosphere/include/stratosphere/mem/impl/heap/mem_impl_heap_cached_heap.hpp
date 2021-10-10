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
#include <stratosphere/mem/impl/mem_impl_common.hpp>

namespace ams::mem::impl::heap {

    class TlsHeapCache;

    class CachedHeap final {
        NON_COPYABLE(CachedHeap);
        private:
            TlsHeapCache *m_tls_heap_cache;
        public:
            constexpr CachedHeap() : m_tls_heap_cache() { /* ... */ }
            ~CachedHeap() { this->Finalize(); }

            ALWAYS_INLINE CachedHeap(CachedHeap &&rhs) : m_tls_heap_cache(rhs.m_tls_heap_cache) {
                rhs.m_tls_heap_cache = nullptr;
            }
            ALWAYS_INLINE CachedHeap &operator=(CachedHeap &&rhs) {
                this->Reset();
                m_tls_heap_cache = rhs.m_tls_heap_cache;
                rhs.m_tls_heap_cache   = nullptr;
                return *this;
            }

            void *Allocate(size_t n);
            void *Allocate(size_t n, size_t align);
            size_t  GetAllocationSize(const void *ptr);
            errno_t Free(void *p);
            errno_t FreeWithSize(void *p, size_t size);
            errno_t Reallocate(void *ptr, size_t size, void **p);
            errno_t Shrink(void *ptr, size_t size);

            void ReleaseAllCache();
            void Finalize();
            bool CheckCache();
            errno_t QueryV(int query, std::va_list vl);
            errno_t Query(int query, ...);

            void Reset() { this->Finalize(); }
            void Reset(TlsHeapCache *thc);
            TlsHeapCache *Release();

            constexpr explicit ALWAYS_INLINE operator bool() const { return m_tls_heap_cache != nullptr; }
    };

}
