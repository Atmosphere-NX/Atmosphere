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
#include "mem_impl_heap_tls_heap_cache.hpp"
#include "mem_impl_heap_tls_heap_central.hpp"

namespace ams::mem::impl::heap {

    TlsHeapCache::TlsHeapCache(TlsHeapCentral *central, u32 option) {
        /* Choose function impls based on option. */
        if ((option & HeapOption_DisableCache) != 0) {
            this->allocate            = AllocateImpl<false>;
            this->allocate_aligned    = AllocateAlignedImpl<false>;
            this->free                = FreeImpl<false>;
            this->free_with_size      = FreeWithSizeImpl<false>;
            this->get_allocation_size = GetAllocationSizeImpl<false>;
            this->reallocate          = ReallocateImpl<false>;
            this->shrink              = ShrinkImpl<false>;
        } else {
            this->allocate            = AllocateImpl<true>;
            this->allocate_aligned    = AllocateAlignedImpl<true>;
            this->free                = FreeImpl<true>;
            this->free_with_size      = FreeWithSizeImpl<true>;
            this->get_allocation_size = GetAllocationSizeImpl<true>;
            this->reallocate          = ReallocateImpl<true>;
            this->shrink              = ShrinkImpl<true>;
        }

        /* Generate random bytes to mangle pointers. */
        if (auto err = gen_random(std::addressof(this->mangle_val), sizeof(this->mangle_val)); err != 0) {
            s64 epoch_time;
            epochtime(std::addressof(epoch_time));
            this->mangle_val = reinterpret_cast<uintptr_t>(std::addressof(epoch_time)) ^ static_cast<u64>(epoch_time);
        }

        /* Set member variables. */
        this->central           = central;
        this->total_heap_size   = central->GetTotalHeapSize();
        this->heap_option       = option;
        this->total_cached_size = 0;
        this->largest_class     = 0;

        /* Setup chunks. */
        for (size_t i = 0; i < TlsHeapStatic::NumClassInfo; i++) {
            this->small_mem_lists[i] = nullptr;
            this->cached_size[i]     = 0;
            this->chunk_count[i]     = 1;
        }

        /* Set fixed chunk counts for particularly small chunks. */
        this->chunk_count[1] = MaxChunkCount;
        this->chunk_count[2] = MaxChunkCount;
        this->chunk_count[3] = MaxChunkCount;
        this->chunk_count[4] = MaxChunkCount / 2;
        this->chunk_count[5] = MaxChunkCount / 2;
        this->chunk_count[6] = MaxChunkCount / 2;
        this->chunk_count[7] = MaxChunkCount / 4;
        this->chunk_count[8] = MaxChunkCount / 4;
        this->chunk_count[9] = MaxChunkCount / 4;
    }

    void TlsHeapCache::Finalize() {
        /* Free all small mem lists. */
        this->ReleaseAllCache();

        /* Remove this cache from the owner central heap. */
        this->central->RemoveThreadCache(this);
        this->central->UncacheSmallMemory(this);
    }

    bool TlsHeapCache::CheckCache() const {
        for (size_t i = 0; i < util::size(this->small_mem_lists); i++) {
            void *ptr = this->small_mem_lists[i];
            if (ptr) {
                s64 depth = -static_cast<s64>(this->cached_size[i] / TlsHeapStatic::GetChunkSize(i));
                while (ptr) {
                    ptr = *reinterpret_cast<void **>(this->ManglePointer(ptr));
                    if ((++depth) == 0) {
                        AMS_ASSERT(ptr == nullptr);
                        break;
                    }
                }
            }
        }

        return true;
    }

    void TlsHeapCache::ReleaseAllCache() {
        for (size_t i = 0; i < util::size(this->small_mem_lists); i++) {
            if (this->small_mem_lists[i]) {
                this->central->UncacheSmallMemoryList(this, this->small_mem_lists[i]);
                this->small_mem_lists[i] = nullptr;
                this->cached_size[i]     = 0;
            }
        }

        this->total_cached_size = 0;
        this->largest_class     = 0;
    }

    template<>
    void *TlsHeapCache::AllocateImpl<false>(TlsHeapCache *_this, size_t size) {
        /* Validate allocation size. */
        if (size == 0 || size > MaxSize) {
            return nullptr;
        }

        if (const size_t cls = TlsHeapStatic::GetClassFromSize(size); cls != 0) {
            AMS_ASSERT(cls < TlsHeapStatic::NumClassInfo);
            return _this->central->CacheSmallMemory(cls);
        } else {
            /* If allocating a huge size, release our cache. */
            if (size >= _this->total_heap_size / 4) {
                _this->ReleaseAllCache();
            }
            return _this->central->CacheLargeMemory(size);
        }
    }

    template<>
    void *TlsHeapCache::AllocateImpl<true>(TlsHeapCache *_this, size_t size) {
        /* Validate allocation size. */
        if (size == 0 || size > MaxSize) {
            return nullptr;
        }

        if (size_t cls = TlsHeapStatic::GetClassFromSize(size); cls != 0) {
            AMS_ASSERT(cls < TlsHeapStatic::NumClassInfo);
            /* Allocate a chunk. */
            void *ptr = _this->small_mem_lists[cls];
            if (ptr == nullptr) {
                const size_t prev_cls = cls;
                size_t count = _this->chunk_count[cls];

                size_t n = _this->central->CacheSmallMemoryList(_this, std::addressof(cls), count, std::addressof(ptr));
                if (n == 0) {
                    return nullptr;
                }

                if (cls == prev_cls) {
                    if (count < MaxChunkCount) {
                        count++;
                    }
                    _this->chunk_count[cls] = std::max(count, n);
                } else {
                    AMS_ASSERT(n == 1);
                }

                const size_t csize = TlsHeapStatic::GetChunkSize(cls) * (n - 1);
                _this->cached_size[cls] += csize;
                if (_this->cached_size[cls] > _this->cached_size[_this->largest_class]) {
                    _this->largest_class = cls;
                }
                _this->total_cached_size += csize;
            }

            /* Demangle our pointer, update free list. */
            ptr = _this->ManglePointer(ptr);
            _this->small_mem_lists[cls] = *reinterpret_cast<void **>(ptr);

            return ptr;
        } else {
            /* If allocating a huge size, release our cache. */
            if (size >= _this->total_heap_size / 4) {
                _this->ReleaseAllCache();
            }
            return _this->central->CacheLargeMemory(size);
        }
    }

    template<>
    void *TlsHeapCache::AllocateAlignedImpl<false>(TlsHeapCache *_this, size_t size, size_t align) {
        /* Ensure valid alignment. */
        if (!util::IsPowerOfTwo(align)) {
            return nullptr;
        }

        /* NOTE: Nintendo does not check size == 0 here, despite doing so in Alloc */
        if (size > MaxSize) {
            return nullptr;
        }

        /* Handle big alignment. */
        if (align > TlsHeapStatic::PageSize) {
            return _this->central->CacheLargeMemoryWithBigAlign(util::AlignUp(size, TlsHeapStatic::PageSize), align);
        }

        const size_t real_size = TlsHeapStatic::GetRealSizeFromSizeAndAlignment(util::AlignUp(size, align), align);

        if (const size_t cls = TlsHeapStatic::GetClassFromSize(real_size); cls != 0) {
            if (real_size == 0) {
                return nullptr;
            }

            AMS_ASSERT(cls < TlsHeapStatic::NumClassInfo);
            return _this->central->CacheSmallMemory(cls, align);
        } else {
            /* If allocating a huge size, release our cache. */
            if (real_size >= _this->total_heap_size / 4) {
                _this->ReleaseAllCache();
            }
            return _this->central->CacheLargeMemory(real_size);
        }
    }

    template<>
    void *TlsHeapCache::AllocateAlignedImpl<true>(TlsHeapCache *_this, size_t size, size_t align) {
        /* Ensure valid alignment. */
        if (!util::IsPowerOfTwo(align)) {
            return nullptr;
        }

        /* NOTE: Nintendo does not check size == 0 here, despite doing so in Alloc */
        if (size > MaxSize) {
            return nullptr;
        }

        /* Handle big alignment. */
        if (align > TlsHeapStatic::PageSize) {
            return _this->central->CacheLargeMemoryWithBigAlign(util::AlignUp(size, TlsHeapStatic::PageSize), align);
        }

        const size_t real_size = TlsHeapStatic::GetRealSizeFromSizeAndAlignment(util::AlignUp(size, align), align);

        if (size_t cls = TlsHeapStatic::GetClassFromSize(real_size); cls != 0) {
            if (real_size == 0) {
                return nullptr;
            }

            AMS_ASSERT(cls < TlsHeapStatic::NumClassInfo);

            /* Allocate a chunk. */
            void *ptr = _this->small_mem_lists[cls];
            if (ptr == nullptr) {
                const size_t prev_cls = cls;
                size_t count = _this->chunk_count[cls];

                size_t n = _this->central->CacheSmallMemoryList(_this, std::addressof(cls), count, std::addressof(ptr), align);
                if (n == 0) {
                    return nullptr;
                }

                if (cls == prev_cls) {
                    if (count < MaxChunkCount) {
                        count++;
                    }
                    _this->chunk_count[cls] = std::max(count, n);
                } else {
                    AMS_ASSERT(n == 1);
                }

                const s32 csize = TlsHeapStatic::GetChunkSize(cls) * (n - 1);
                _this->total_cached_size += csize;
                _this->cached_size[cls] += csize;
                if (_this->cached_size[cls] > _this->cached_size[_this->largest_class]) {
                    _this->largest_class = cls;
                }
            }

            /* Demangle our pointer, update free list. */
            ptr = _this->ManglePointer(ptr);
            _this->small_mem_lists[cls] = *reinterpret_cast<void **>(ptr);

            return ptr;
        } else {
            /* If allocating a huge size, release our cache. */
            if (size >= _this->total_heap_size / 4) {
                _this->ReleaseAllCache();
            }
            return _this->central->CacheLargeMemory(size);
        }
    }

    template<>
    errno_t TlsHeapCache::FreeImpl<false>(TlsHeapCache *_this, void *ptr) {
        const size_t cls = _this->central->GetClassFromPointer(ptr);
        if (cls == 0) {
            return _this->central->UncacheLargeMemory(ptr);
        }

        AMS_ASSERT(cls < TlsHeapStatic::NumClassInfo);

        if (static_cast<s32>(cls) >= 0) {
            return _this->central->UncacheSmallMemory(ptr);
        } else if (ptr == nullptr) {
            return 0;
        } else {
            return EFAULT;
        }
    }

    template<>
    errno_t TlsHeapCache::FreeImpl<true>(TlsHeapCache *_this, void *ptr) {
        const size_t cls = _this->central->GetClassFromPointer(ptr);
        if (cls == 0) {
            return _this->central->UncacheLargeMemory(ptr);
        }

        AMS_ASSERT(cls < TlsHeapStatic::NumClassInfo);

        if (static_cast<s32>(cls) >= 0) {
            *reinterpret_cast<void **>(ptr) = _this->small_mem_lists[cls];
            _this->small_mem_lists[cls] = _this->ManglePointer(ptr);

            const s32 csize = TlsHeapStatic::GetChunkSize(cls);
            _this->total_cached_size += csize;
            _this->cached_size[cls]  += csize;
            if (_this->cached_size[cls] > _this->cached_size[_this->largest_class]) {
                _this->largest_class = cls;
            }

            errno_t err = 0;
            if (!_this->central->CheckCachedSize(_this->total_cached_size)) {
                _this->central->UncacheSmallMemoryList(_this, _this->small_mem_lists[_this->largest_class]);
                _this->small_mem_lists[_this->largest_class] = nullptr;
                _this->total_cached_size -= _this->cached_size[_this->largest_class];
                _this->cached_size[_this->largest_class] = 0;

                s32 largest_class = 0;
                s32 biggest_size = -1;
                for (size_t i = 0; i < TlsHeapStatic::NumClassInfo; i++) {
                    if (biggest_size < _this->cached_size[i]) {
                        biggest_size = _this->cached_size[i];
                        largest_class = static_cast<s32>(i);
                    }
                }
                _this->largest_class = largest_class;
            }
            return err;
        } else if (ptr == nullptr) {
            return 0;
        } else {
            return EFAULT;
        }
    }

    template<>
    errno_t TlsHeapCache::FreeWithSizeImpl<false>(TlsHeapCache *_this, void *ptr, size_t size) {
        if (ptr == nullptr) {
            return 0;
        }

        const size_t cls = TlsHeapStatic::GetClassFromSize(size);
        if (cls == 0) {
            return _this->central->UncacheLargeMemory(ptr);
        } else {
            return _this->central->UncacheSmallMemory(ptr);
        }
    }

    template<>
    errno_t TlsHeapCache::FreeWithSizeImpl<true>(TlsHeapCache *_this, void *ptr, size_t size) {
        if (ptr == nullptr) {
            return 0;
        }

        const size_t cls = TlsHeapStatic::GetClassFromSize(size);
        if (cls == 0) {
            return _this->central->UncacheLargeMemory(ptr);
        } else {
            *reinterpret_cast<void **>(ptr) = _this->small_mem_lists[cls];
            _this->small_mem_lists[cls] = _this->ManglePointer(ptr);

            const s32 csize = TlsHeapStatic::GetChunkSize(cls);
            _this->total_cached_size += csize;
            _this->cached_size[cls]  += csize;
            if (_this->cached_size[cls] > _this->cached_size[_this->largest_class]) {
                _this->largest_class = cls;
            }

            errno_t err = 0;
            if (!_this->central->CheckCachedSize(_this->total_cached_size)) {
                _this->central->UncacheSmallMemoryList(_this, _this->small_mem_lists[_this->largest_class]);
                _this->small_mem_lists[_this->largest_class] = nullptr;
                _this->total_cached_size -= _this->cached_size[_this->largest_class];
                _this->cached_size[_this->largest_class] = 0;

                s32 largest_class = 0;
                s32 biggest_size = -1;
                for (size_t i = 0; i < TlsHeapStatic::NumClassInfo; i++) {
                    if (biggest_size < _this->cached_size[i]) {
                        biggest_size = _this->cached_size[i];
                        largest_class = static_cast<s32>(i);
                    }
                }
                _this->largest_class = largest_class;
            }
            return err;
        }
    }

    template<>
    size_t TlsHeapCache::GetAllocationSizeImpl<false>(TlsHeapCache *_this, const void *ptr) {
        return _this->GetAllocationSizeCommonImpl(ptr);
    }

    template<>
    size_t TlsHeapCache::GetAllocationSizeImpl<true>(TlsHeapCache *_this, const void *ptr) {
        return _this->GetAllocationSizeCommonImpl(ptr);
    }

    size_t TlsHeapCache::GetAllocationSizeCommonImpl(const void *ptr) const {
        const s32 cls = this->central->GetClassFromPointer(ptr);
        if (cls > 0) {
            if (!util::IsAligned(ptr, alignof(u64))) {
                /* All pointers we allocate have alignment at least 8. */
                return 0;
            }

            /* Validate class. */
            AMS_ASSERT(cls < static_cast<s32>(TlsHeapStatic::NumClassInfo));
            if (cls < 0) {
                return 0;
            }

            return TlsHeapStatic::GetChunkSize(cls);
        } else if (ptr != nullptr) {
            return this->central->GetAllocationSize(ptr);
        } else {
            return 0;
        }
    }

    template<>
    errno_t TlsHeapCache::ReallocateImpl<false>(TlsHeapCache *_this, void *ptr, size_t size, void **p) {
        AMS_ASSERT(ptr != nullptr && size != 0);
        if (size > MaxSize) {
            return ENOMEM;
        }

        size_t alloc_size, copy_size;

        const s32 cls_from_size = TlsHeapStatic::GetClassFromSize(size);
        const s32 cls_from_ptr  = _this->central->GetClassFromPointer(ptr);
        if (cls_from_ptr < 0) {
            /* error case. */
            return EFAULT;
        } else if (cls_from_size) {
            if (cls_from_ptr > 0) {
                if (cls_from_size <= cls_from_ptr) {
                    *p = ptr;
                    return 0;
                } else {
                    alloc_size = size;
                    copy_size  = TlsHeapStatic::GetChunkSize(cls_from_ptr);
                }
            } else /* if (cls_from_ptr == 0) */ {
                alloc_size = size;
                copy_size  = size;
            }
        } else if (cls_from_ptr == 0) {
            return _this->central->ReallocateLargeMemory(ptr, size, p);
        } else /* if (cls_from_ptr > 0) */ {
            alloc_size = size;
            copy_size  = TlsHeapStatic::GetChunkSize(cls_from_ptr);
        }

        *p = AllocateImpl<false>(_this, alloc_size);
        if (*p == nullptr) {
            return ENOMEM;
        }
        std::memcpy(*p, ptr, copy_size);
        return FreeImpl<false>(_this, ptr);
    }

    template<>
    errno_t TlsHeapCache::ReallocateImpl<true>(TlsHeapCache *_this, void *ptr, size_t size, void **p) {
        AMS_ASSERT(ptr != nullptr && size != 0);
        if (size > MaxSize) {
            return ENOMEM;
        }

        size_t alloc_size, copy_size;

        const s32 cls_from_size = TlsHeapStatic::GetClassFromSize(size);
        const s32 cls_from_ptr  = _this->central->GetClassFromPointer(ptr);
        if (cls_from_ptr < 0) {
            /* error case. */
            return EFAULT;
        } else if (cls_from_size) {
            if (cls_from_ptr > 0) {
                if (cls_from_size <= cls_from_ptr) {
                    *p = ptr;
                    return 0;
                } else {
                    alloc_size = size;
                    copy_size  = TlsHeapStatic::GetChunkSize(cls_from_ptr);
                }
            } else /* if (cls_from_ptr == 0) */ {
                alloc_size = size;
                copy_size  = size;
            }
        } else if (cls_from_ptr == 0) {
            return _this->central->ReallocateLargeMemory(ptr, size, p);
        } else /* if (cls_from_ptr > 0) */ {
            alloc_size = size;
            copy_size  = TlsHeapStatic::GetChunkSize(cls_from_ptr);
        }

        *p = AllocateImpl<true>(_this, alloc_size);
        if (*p == nullptr) {
            return ENOMEM;
        }
        std::memcpy(*p, ptr, copy_size);
        return FreeImpl<true>(_this, ptr);
    }

    template<>
    errno_t TlsHeapCache::ShrinkImpl<false>(TlsHeapCache *_this, void *ptr, size_t size) {
        return _this->ShrinkCommonImpl(ptr, size);
    }

    template<>
    errno_t TlsHeapCache::ShrinkImpl<true>(TlsHeapCache *_this, void *ptr, size_t size) {
        return _this->ShrinkCommonImpl(ptr, size);
    }

    errno_t TlsHeapCache::ShrinkCommonImpl(void *ptr, size_t size) const {
        AMS_ASSERT(ptr != nullptr && size != 0);
        if (size > MaxSize) {
            return ENOMEM;
        }

        const s32 cls_from_size = TlsHeapStatic::GetClassFromSize(size);
        const s32 cls_from_ptr  = this->central->GetClassFromPointer(ptr);
        if (cls_from_ptr) {
            if (cls_from_ptr <= 0) {
                return EFAULT;
            } else if (cls_from_size && cls_from_size <= cls_from_ptr) {
                return 0;
            } else {
                return EINVAL;
            }
        } else if (cls_from_size) {
            return this->central->ShrinkLargeMemory(ptr, TlsHeapStatic::PageSize);
        } else {
            return this->central->ShrinkLargeMemory(ptr, size);
        }
    }

}
