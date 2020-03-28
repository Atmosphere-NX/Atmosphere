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
#pragma once
#include <stratosphere.hpp>
#include "mem_impl_heap_platform.hpp"
#include "mem_impl_heap_tls_heap_static.hpp"

namespace ams::mem::impl::heap {

    class TlsHeapCentral;

    #define FOREACH_TLS_HEAP_CACHE_FUNC(HANDLER)                                                    \
        HANDLER(void *,  Allocate,          allocate,            size_t size);                      \
        HANDLER(void *,  AllocateAligned,   allocate_aligned,    size_t size, size_t align);        \
        HANDLER(errno_t, Free,              free,                void *ptr);                        \
        HANDLER(errno_t, FreeWithSize,      free_with_size,      void *ptr, size_t size);           \
        HANDLER(size_t,  GetAllocationSize, get_allocation_size, const void *ptr);                  \
        HANDLER(errno_t, Reallocate,        reallocate,          void *ptr, size_t size, void **p); \
        HANDLER(errno_t, Shrink,            shrink,              void *ptr, size_t size);

    class TlsHeapCache {
        public:
            static constexpr size_t MaxChunkCount = BITSIZEOF(u64);
        public:
            #define TLS_HEAP_CACHE_DECLARE_TYPEDEF(RETURN, NAME, MEMBER_NAME, ...) \
                using NAME##Func = RETURN (*)(TlsHeapCache *, ## __VA_ARGS__)

            FOREACH_TLS_HEAP_CACHE_FUNC(TLS_HEAP_CACHE_DECLARE_TYPEDEF)

            #undef TLS_HEAP_CACHE_DECLARE_TYPEDEF
        private:
            #define TLS_HEAP_CACHE_DECLARE_MEMBER(RETURN, NAME, MEMBER_NAME, ...) \
                NAME##Func MEMBER_NAME;

            FOREACH_TLS_HEAP_CACHE_FUNC(TLS_HEAP_CACHE_DECLARE_MEMBER)

            #undef TLS_HEAP_CACHE_DECLARE_MEMBER

            uintptr_t        mangle_val;
            TlsHeapCentral  *central;
            size_t           total_heap_size;
            u32              heap_option;
            s32              total_cached_size;
            s32              largest_class;
            void            *small_mem_lists[TlsHeapStatic::NumClassInfo];
            s32              cached_size[TlsHeapStatic::NumClassInfo];
            u8               chunk_count[TlsHeapStatic::NumClassInfo];
        public:
            TlsHeapCache(TlsHeapCentral *central, u32 option);
            void Finalize();

            void *ManglePointer(void *ptr) const {
                return reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(ptr) ^ this->mangle_val);
            }

            bool CheckCache() const;
            void ReleaseAllCache();

        public:
            /* TODO: Better handler with type info to macro this? */
            ALWAYS_INLINE void    *Allocate(size_t size)                       { return this->allocate(this, size); }
            ALWAYS_INLINE void    *Allocate(size_t size, size_t align)         { return this->allocate_aligned(this, size, align); }
            ALWAYS_INLINE errno_t Free(void *ptr)                              { return this->free(this, ptr); }
            ALWAYS_INLINE errno_t FreeWithSize(void *ptr, size_t size)         { return this->free_with_size(this, ptr, size); }
            ALWAYS_INLINE size_t  GetAllocationSize(const void *ptr)           { return this->get_allocation_size(this, ptr); }
            ALWAYS_INLINE errno_t Reallocate(void *ptr, size_t size, void **p) { return this->reallocate(this, ptr, size, p); }
            ALWAYS_INLINE errno_t Shrink(void *ptr, size_t size)               { return this->shrink(this, ptr, size); }
        private:
            #define TLS_HEAP_CACHE_DECLARE_TEMPLATE(RETURN, NAME, MEMBER_NAME, ...) \
            template<bool Cache> static RETURN NAME##Impl(TlsHeapCache *_this, ## __VA_ARGS__ )

            FOREACH_TLS_HEAP_CACHE_FUNC(TLS_HEAP_CACHE_DECLARE_TEMPLATE)

            #undef TLS_HEAP_CACHE_DECLARE_TEMPLATE

            size_t GetAllocationSizeCommonImpl(const void *ptr) const;
            errno_t ShrinkCommonImpl(void *ptr, size_t size) const;
    };

    #define TLS_HEAP_CACHE_DECLARE_INSTANTIATION(RETURN, NAME, MEMBER_NAME, ...)                             \
        template<> RETURN TlsHeapCache::NAME##Impl<false>(TlsHeapCache *_this, ##__VA_ARGS__);  \
        template<> RETURN TlsHeapCache::NAME##Impl<true>(TlsHeapCache *_this, ##__VA_ARGS__)

    FOREACH_TLS_HEAP_CACHE_FUNC(TLS_HEAP_CACHE_DECLARE_INSTANTIATION)

    #undef FOREACH_TLS_HEAP_CACHE_FUNC


}