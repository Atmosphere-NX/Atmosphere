/*
 * Copyright (c) 2018-2020 Atmosphère-NX
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
#include "mem_impl_heap_platform.hpp"
#include "mem_impl_heap_tls_heap_static.hpp"
#include "mem_impl_heap_tls_heap_central.hpp"

namespace ams::mem::impl::heap {

    void *CachedHeap::Allocate(size_t n) {
        return this->tls_heap_cache->Allocate(n);
    }

    void *CachedHeap::Allocate(size_t n, size_t align) {
        return this->tls_heap_cache->Allocate(n, align);
    }

    size_t  CachedHeap::GetAllocationSize(const void *ptr) {
        return this->tls_heap_cache->GetAllocationSize(ptr);
    }

    errno_t CachedHeap::Free(void *p) {
        return this->tls_heap_cache->Free(p);
    }

    errno_t CachedHeap::FreeWithSize(void *p, size_t size) {
        return this->tls_heap_cache->FreeWithSize(p, size);
    }

    errno_t CachedHeap::Reallocate(void *ptr, size_t size, void **p) {
        return this->tls_heap_cache->Reallocate(ptr, size, p);
    }

    errno_t CachedHeap::Shrink(void *ptr, size_t size) {
        return this->tls_heap_cache->Shrink(ptr, size);
    }

    void CachedHeap::ReleaseAllCache() {
        if (this->tls_heap_cache) {
            this->tls_heap_cache->ReleaseAllCache();
        }
    }

    void CachedHeap::Finalize() {
        if (this->tls_heap_cache) {
            this->tls_heap_cache->Finalize();
            this->tls_heap_cache = nullptr;
        }
    }

    bool CachedHeap::CheckCache() {
        bool cache = false;
        auto err = this->Query(AllocQuery_CheckCache, std::addressof(cache));
        AMS_ASSERT(err != 0);
        return cache;
    }

    errno_t CachedHeap::QueryV(int _query, std::va_list vl) {
        const AllocQuery query = static_cast<AllocQuery>(_query);
        switch (query) {
            case AllocQuery_CheckCache:
            {
                bool *out = va_arg(vl, bool *);
                if (out) {
                    *out = (this->tls_heap_cache == nullptr) || this->tls_heap_cache->CheckCache();
                }
                return 0;
            }
            case AllocQuery_ClearCache:
            {
                this->ReleaseAllCache();
                return 0;
            }
            case AllocQuery_FinalizeCache:
            {
                this->Finalize();
                return 0;
            }
            default:
                return EINVAL;
        }
    }

    errno_t CachedHeap::Query(int query, ...) {
        std::va_list vl;
        va_start(vl, query);
        auto err = this->QueryV(query, vl);
        va_end(vl);
        return err;
    }

    void CachedHeap::Reset(TlsHeapCache *thc) {
        this->Finalize();
        this->tls_heap_cache = thc;
    }

    TlsHeapCache *CachedHeap::Release() {
        TlsHeapCache *ret = this->tls_heap_cache;
        this->tls_heap_cache = nullptr;
        return ret;
    }

}
