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

    errno_t CentralHeap::Initialize(void *start, size_t size, u32 option) {
        /* Validate size. */
        if (size == 0 || !util::IsAligned(size, PageSize)) {
            return EINVAL;
        }

        /* Don't allow initializing twice. */
        if (this->start) {
            return EEXIST;
        }

        if (start) {
            /* We were provided with a region to use as backing memory. */
            u8 *aligned_start = reinterpret_cast<u8 *>(util::AlignUp(reinterpret_cast<uintptr_t>(start), PageSize));
            u8 *aligned_end   = reinterpret_cast<u8 *>(util::AlignDown(reinterpret_cast<uintptr_t>(start) + size, PageSize));
            if (aligned_start >= aligned_end) {
                return EINVAL;
            }

            this->start  = aligned_start;
            this->end    = aligned_end;
            this->option = option;
            this->tls_heap_central = new (this->start) TlsHeapCentral;
            if (auto err = this->tls_heap_central->Initialize(this->start, this->end - this->start, false); err != 0) {
                this->tls_heap_central->~TlsHeapCentral();
                this->tls_heap_central = nullptr;
                AMS_ASSERT(err == 0);
                return err;
            }
            this->use_virtual_memory = false;
        } else {
            /* We were not provided with a region to use as backing. */
            void *mem;
            if (auto err = AllocateVirtualMemory(std::addressof(mem), size); err != 0) {
                return err;
            }
            if (!util::IsAligned(reinterpret_cast<uintptr_t>(mem), PageSize)) {
                FreeVirtualMemory(mem, size);
                size += PageSize;
                if (auto err = AllocateVirtualMemory(std::addressof(mem), size); err != 0) {
                    return err;
                }
            }
            this->start  = static_cast<u8 *>(mem);
            this->end    = this->start + size;
            this->option = option;
            void *central = reinterpret_cast<void *>(util::AlignUp(reinterpret_cast<uintptr_t>(mem), PageSize));
            if (auto err = AllocatePhysicalMemory(central, sizeof(TlsHeapCentral)); err != 0) {
                return err;
            }
            this->tls_heap_central = new (central) TlsHeapCentral;
            if (auto err = this->tls_heap_central->Initialize(central, size, true); err != 0) {
                this->tls_heap_central->~TlsHeapCentral();
                this->tls_heap_central = nullptr;
                AMS_ASSERT(err == 0);
                return err;
            }
            this->use_virtual_memory = true;
        }

        return 0;
    }

    void CentralHeap::Finalize() {
        if (this->tls_heap_central) {
            this->tls_heap_central->~TlsHeapCentral();
        }
        if (this->use_virtual_memory) {
            mem::impl::physical_free(util::AlignUp(static_cast<void *>(this->start), PageSize), this->end - this->start);
            mem::impl::virtual_free(this->start, this->end - this->start);
        }
        this->tls_heap_central   = nullptr;
        this->use_virtual_memory = false;
        this->option             = 0;
        this->start              = nullptr;
        this->end                = nullptr;
    }

    void *CentralHeap::Allocate(size_t n, size_t align) {
        if (!util::IsPowerOfTwo(align)) {
            return nullptr;
        }
        if (n > MaxSize) {
            return nullptr;
        }
        if (align > PageSize) {
            return this->tls_heap_central->CacheLargeMemoryWithBigAlign(util::AlignUp(n, PageSize), align);
        }

        const size_t real_size = TlsHeapStatic::GetRealSizeFromSizeAndAlignment(util::AlignUp(n, align), align);
        const auto cls = TlsHeapStatic::GetClassFromSize(real_size);
        if (!cls) {
            return this->tls_heap_central->CacheLargeMemory(real_size);
        }
        if (real_size == 0) {
            return nullptr;
        }
        AMS_ASSERT(cls < TlsHeapStatic::NumClassInfo);
        return this->tls_heap_central->CacheSmallMemory(cls, align);
    }

    size_t CentralHeap::GetAllocationSize(const void *ptr) {
        const auto cls = this->tls_heap_central->GetClassFromPointer(ptr);
        if (cls > 0) {
            /* Check that the pointer has alignment from out allocator. */
            if (!util::IsAligned(reinterpret_cast<uintptr_t>(ptr), MinimumAlignment)) {
                return 0;
            }
            AMS_ASSERT(cls < TlsHeapStatic::NumClassInfo);
            return TlsHeapStatic::GetChunkSize(cls);
        } else if (ptr != nullptr) {
            return this->tls_heap_central->GetAllocationSize(ptr);
        } else {
            return 0;
        }
    }

    errno_t CentralHeap::Free(void *ptr) {
        /* Allow Free(nullptr) */
        if (ptr == nullptr) {
            return 0;
        }

        /* Check that the pointer has alignment from out allocator. */
        if(!util::IsAligned(reinterpret_cast<uintptr_t>(ptr), MinimumAlignment)) {
            AMS_ASSERT(util::IsAligned(reinterpret_cast<uintptr_t>(ptr), MinimumAlignment));
            return EFAULT;
        }

        const auto cls = this->tls_heap_central->GetClassFromPointer(ptr);
        if (cls >= 0) {
            AMS_ASSERT(cls < TlsHeapStatic::NumClassInfo);
            if (cls) {
                return this->tls_heap_central->UncacheSmallMemory(ptr);
            } else {
                return this->tls_heap_central->UncacheLargeMemory(ptr);
            }
        } else {
            AMS_ASSERT(cls >= 0);
            return EFAULT;
        }
    }

    errno_t CentralHeap::FreeWithSize(void *ptr, size_t size) {
        if (TlsHeapStatic::GetClassFromSize(size)) {
            return this->tls_heap_central->UncacheSmallMemory(ptr);
        } else {
            return this->tls_heap_central->UncacheLargeMemory(ptr);
        }
    }

    errno_t CentralHeap::Reallocate(void *ptr, size_t size, void **p) {
        AMS_ASSERT(ptr != nullptr && size != 0);
        if (!size) {
            return EINVAL;
        }
        if (size > MaxSize) {
            return ENOMEM;
        }

        const auto cls_from_size = TlsHeapStatic::GetClassFromSize(size);
        const auto cls_from_ptr  = this->tls_heap_central->GetClassFromPointer(ptr);
        if (cls_from_ptr) {
            if (cls_from_ptr <= 0) {
                return EFAULT;
            } else if (cls_from_size && cls_from_size <= cls_from_ptr) {
                *p = ptr;
                return 0;
            } else {
                const size_t new_chunk_size = TlsHeapStatic::GetChunkSize(cls_from_ptr);
                *p = this->Allocate(new_chunk_size);
                if (*p)  {
                    std::memcpy(*p, ptr, size);
                    return this->tls_heap_central->UncacheSmallMemory(ptr);
                } else {
                    return ENOMEM;
                }
            }
        } else if (cls_from_size) {
            *p = this->Allocate(size);
            if (*p)  {
                std::memcpy(*p, ptr, size);
                return this->tls_heap_central->UncacheLargeMemory(ptr);
            } else {
                return ENOMEM;
            }
        } else {
            return this->tls_heap_central->ReallocateLargeMemory(ptr, size, p);
        }
    }

    errno_t CentralHeap::Shrink(void *ptr, size_t size) {
        AMS_ASSERT(ptr != nullptr && size != 0);
        if (!size) {
            return EINVAL;
        }
        if (size > MaxSize) {
            return ENOMEM;
        }

        const auto cls_from_size = TlsHeapStatic::GetClassFromSize(size);
        const auto cls_from_ptr  = this->tls_heap_central->GetClassFromPointer(ptr);
        if (cls_from_ptr) {
            if (cls_from_ptr <= 0) {
                return EFAULT;
            } else if (cls_from_size && cls_from_size <= cls_from_ptr) {
                return 0;
            } else {
                return EINVAL;
            }
        } else if (cls_from_size) {
            return this->tls_heap_central->ShrinkLargeMemory(ptr, PageSize);
        } else {
            return this->tls_heap_central->ShrinkLargeMemory(ptr, size);
        }
    }

    bool CentralHeap::MakeCache(CachedHeap *cached_heap) {
        if (cached_heap == nullptr) {
            return false;
        }

        AMS_ASSERT(this->tls_heap_central != nullptr);
        const auto cls = TlsHeapStatic::GetClassFromSize(sizeof(*cached_heap));
        void *tls_heap_cache = this->tls_heap_central->CacheSmallMemoryForSystem(cls);
        if (tls_heap_cache == nullptr) {
            return false;
        }

        new (tls_heap_cache) TlsHeapCache(this->tls_heap_central, this->option);
        if (this->tls_heap_central->AddThreadCache(reinterpret_cast<TlsHeapCache *>(tls_heap_cache)) != 0) {
            this->tls_heap_central->UncacheSmallMemory(tls_heap_cache);
            return false;
        }

        cached_heap->Reset(reinterpret_cast<TlsHeapCache *>(tls_heap_cache));
        return true;
    }

    errno_t CentralHeap::WalkAllocatedPointers(HeapWalkCallback callback, void *user_data) {
        if (!callback || !this->tls_heap_central) {
            return EINVAL;
        }
        return this->tls_heap_central->WalkAllocatedPointers(callback, user_data);
    }

    errno_t CentralHeap::QueryV(int query, std::va_list vl) {
        return this->QueryVImpl(query, std::addressof(vl));
    }

    errno_t CentralHeap::Query(int query, ...) {
        std::va_list vl;
        va_start(vl, query);
        auto err = this->QueryVImpl(query, std::addressof(vl));
        va_end(vl);
        return err;
    }

    errno_t CentralHeap::QueryVImpl(int _query, std::va_list *vl_ptr) {
        const AllocQuery query = static_cast<AllocQuery>(_query);
        switch (query) {
            case AllocQuery_Dump:
            case AllocQuery_DumpJson:
            {
                auto dump_mode = static_cast<DumpMode>(va_arg(*vl_ptr, int));
                auto fd = va_arg(*vl_ptr, int);
                if (this->tls_heap_central) {
                    this->tls_heap_central->Dump(dump_mode, fd, query == AllocQuery_DumpJson);
                }
                return 0;
            }
            case AllocQuery_PageSize:
            {
                size_t *out = va_arg(*vl_ptr, size_t *);
                if (out) {
                    *out = PageSize;
                }
                return 0;
            }
            case AllocQuery_AllocatedSize:
            case AllocQuery_FreeSize:
            case AllocQuery_SystemSize:
            case AllocQuery_MaxAllocatableSize:
            {
                size_t *out = va_arg(*vl_ptr, size_t *);
                if (!out) {
                    return 0;
                }
                if (!this->tls_heap_central) {
                    *out = 0;
                    return 0;
                }
                TlsHeapMemStats stats;
                this->tls_heap_central->GetMemStats(std::addressof(stats));
                switch (query) {
                    case AllocQuery_AllocatedSize:
                    default:
                        *out = stats.allocated_size;
                        break;
                    case AllocQuery_FreeSize:
                        *out = stats.free_size;
                        break;
                    case AllocQuery_SystemSize:
                        *out = stats.system_size;
                        break;
                    case AllocQuery_MaxAllocatableSize:
                        *out = stats.max_allocatable_size;
                        break;
                }
                return 0;
            }
            case AllocQuery_IsClean:
            {
                int *out = va_arg(*vl_ptr, int *);
                if (out) {
                    *out = !this->tls_heap_central || this->tls_heap_central->IsClean();
                }
                return 0;
            }
            case AllocQuery_HeapHash:
            {
                HeapHash *out = va_arg(*vl_ptr, HeapHash *);
                if (out) {
                    if (this->tls_heap_central) {
                        this->tls_heap_central->CalculateHeapHash(out);
                    } else {
                        *out = {};
                    }
                }
                return 0;
            }
            case AllocQuery_UnifyFreeList:
                /* NOTE: Nintendo does not check that the ptr is not null for this query, even though they do for other queries. */
                this->tls_heap_central->IsClean();
                return 0;
            case AllocQuery_SetColor:
            {
                /* NOTE: Nintendo does not check that the ptr is not null for this query, even though they do for other queries. */
                void *ptr = va_arg(*vl_ptr, void *);
                int color = va_arg(*vl_ptr, int);
                return this->tls_heap_central->SetColor(ptr, color);
            }
            case AllocQuery_GetColor:
            {
                /* NOTE: Nintendo does not check that the ptr is not null for this query, even though they do for other queries. */
                void *ptr = va_arg(*vl_ptr, void *);
                int *out = va_arg(*vl_ptr, int *);
                return this->tls_heap_central->GetColor(ptr, out);
            }
            case AllocQuery_SetName:
            {
                /* NOTE: Nintendo does not check that the ptr is not null for this query, even though they do for other queries. */
                void *ptr = va_arg(*vl_ptr, void *);
                const char *name = va_arg(*vl_ptr, const char *);
                return this->tls_heap_central->SetName(ptr, name);
            }
            case AllocQuery_GetName:
            {
                /* NOTE: Nintendo does not check that the ptr is not null for this query, even though they do for other queries. */
                void *ptr = va_arg(*vl_ptr, void *);
                char *dst = va_arg(*vl_ptr, char *);
                size_t dst_size = va_arg(*vl_ptr, size_t);
                return this->tls_heap_central->GetName(ptr, dst, dst_size);
            }
            case AllocQuery_FreeSizeMapped:
            case AllocQuery_MaxAllocatableSizeMapped:
            {
                /* NOTE: Nintendo does not check that the ptr is not null for this query, even though they do for other queries. */
                size_t *out = va_arg(*vl_ptr, size_t *);
                size_t free_size;
                size_t max_allocatable_size;
                auto err = this->tls_heap_central->GetMappedMemStats(std::addressof(free_size), std::addressof(max_allocatable_size));
                if (err == 0) {
                    if (query == AllocQuery_FreeSizeMapped) {
                        *out = free_size;
                    } else {
                        *out = max_allocatable_size;
                    }
                }
                return err;
            }
            default:
                return EINVAL;
        }
    }

}
