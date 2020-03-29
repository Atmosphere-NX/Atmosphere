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
#include "impl/mem_impl_platform.hpp"
#include "impl/heap/mem_impl_heap_tls_heap_static.hpp"
#include "impl/heap/mem_impl_heap_tls_heap_cache.hpp"
#include "impl/heap/mem_impl_heap_tls_heap_central.hpp"

namespace ams::mem {

    constexpr inline size_t DefaultAlignment     = alignof(std::max_align_t);
    constexpr inline size_t MinimumAllocatorSize = 16_KB;

    namespace {

        void ThreadDestroy(uintptr_t arg) {
            if (arg) {
                reinterpret_cast<impl::heap::TlsHeapCache *>(arg)->Finalize();
            }
        }

        ALWAYS_INLINE impl::heap::CentralHeap *GetCentral(const impl::InternalCentralHeapStorage *storage) {
            return reinterpret_cast<impl::heap::CentralHeap *>(const_cast<impl::InternalCentralHeapStorage *>(storage));
        }

        ALWAYS_INLINE impl::heap::CentralHeap *GetCentral(const impl::InternalCentralHeapStorage &storage) {
            return GetCentral(std::addressof(storage));
        }

        ALWAYS_INLINE void GetCache(impl::heap::CentralHeap *central, os::TlsSlot slot) {
            impl::heap::CachedHeap tmp_cache;

            if (central->MakeCache(std::addressof(tmp_cache))) {
                impl::heap::TlsHeapCache *cache = tmp_cache.Release();
                os::SetTlsValue(slot, reinterpret_cast<uintptr_t>(cache));
            }
        }

        struct InternalHash {
            size_t allocated_count;
            size_t allocated_size;
            crypto::Sha1Generator sha1;
        };

        int InternalHashCallback(void *ptr, size_t size, void *user_data) {
            InternalHash *hash = reinterpret_cast<InternalHash *>(user_data);
            hash->sha1.Update(reinterpret_cast<void *>(std::addressof(ptr)),  sizeof(ptr));
            hash->sha1.Update(reinterpret_cast<void *>(std::addressof(size)), sizeof(size));
            hash->allocated_count++;
            hash->allocated_size += size;
            return 1;
        }

    }

    StandardAllocator::StandardAllocator() : initialized(false), enable_thread_cache(false), unused(0) {
        static_assert(sizeof(impl::heap::CentralHeap) <= sizeof(this->central_heap_storage));
        new (std::addressof(this->central_heap_storage)) impl::heap::CentralHeap;
    }

    StandardAllocator::StandardAllocator(void *mem, size_t size) : StandardAllocator() {
        this->Initialize(mem, size);
    }

    StandardAllocator::StandardAllocator(void *mem, size_t size, bool enable_cache) : StandardAllocator() {
        this->Initialize(mem, size, enable_cache);
    }

    void StandardAllocator::Initialize(void *mem, size_t size) {
        this->Initialize(mem, size, false);
    }

    void StandardAllocator::Initialize(void *mem, size_t size, bool enable_cache) {
        AMS_ABORT_UNLESS(!this->initialized);

        const uintptr_t aligned_start = util::AlignUp(reinterpret_cast<uintptr_t>(mem), impl::heap::TlsHeapStatic::PageSize);
        const uintptr_t aligned_end   = util::AlignDown(reinterpret_cast<uintptr_t>(mem) + size, impl::heap::TlsHeapStatic::PageSize);
        const size_t    aligned_size  = aligned_end - aligned_start;

        if (mem == nullptr) {
            AMS_ABORT_UNLESS(os::IsVirtualAddressMemoryEnabled());
            AMS_ABORT_UNLESS(GetCentral(this->central_heap_storage)->Initialize(nullptr, size, 0) == 0);
        } else {
            AMS_ABORT_UNLESS(aligned_start < aligned_end);
            AMS_ABORT_UNLESS(aligned_size >= MinimumAllocatorSize);
            AMS_ABORT_UNLESS(GetCentral(this->central_heap_storage)->Initialize(reinterpret_cast<void *>(aligned_start), aligned_size, 0) == 0);
        }

        this->enable_thread_cache = enable_cache;
        if (this->enable_thread_cache) {
            R_ABORT_UNLESS(os::AllocateTlsSlot(std::addressof(this->tls_slot), ThreadDestroy));
        }

        this->initialized = true;
    }

    void StandardAllocator::Finalize() {
        AMS_ABORT_UNLESS(this->initialized);

        if (this->enable_thread_cache) {
            os::FreeTlsSlot(this->tls_slot);
        }

        GetCentral(this->central_heap_storage)->Finalize();

        this->initialized = false;
    }

    void *StandardAllocator::Allocate(size_t size) {
        AMS_ASSERT(this->initialized);
        return this->Allocate(size, DefaultAlignment);
    }

    void *StandardAllocator::Allocate(size_t size, size_t alignment) {
        AMS_ASSERT(this->initialized);

        impl::heap::TlsHeapCache *heap_cache = nullptr;
        if (this->enable_thread_cache) {
            heap_cache = reinterpret_cast<impl::heap::TlsHeapCache *>(os::GetTlsValue(this->tls_slot));
            if (!heap_cache) {
                GetCache(GetCentral(this->central_heap_storage), this->tls_slot);
                heap_cache = reinterpret_cast<impl::heap::TlsHeapCache *>(os::GetTlsValue(this->tls_slot));
            }
        }

        void *ptr = nullptr;
        if (heap_cache) {
            ptr = heap_cache->Allocate(size, alignment);
            if (ptr) {
                return ptr;
            }

            impl::heap::CachedHeap cache;
            cache.Reset(heap_cache);
            cache.Query(impl::AllocQuery_FinalizeCache);
            os::SetTlsValue(this->tls_slot, 0);
        }

        return GetCentral(this->central_heap_storage)->Allocate(size, alignment);
    }

    void StandardAllocator::Free(void *ptr) {
        AMS_ASSERT(this->initialized);

        if (ptr == nullptr) {
            return;
        }

        if (this->enable_thread_cache) {
            impl::heap::TlsHeapCache *heap_cache = reinterpret_cast<impl::heap::TlsHeapCache *>(os::GetTlsValue(this->tls_slot));
            if (heap_cache) {
                heap_cache->Free(ptr);
                return;
            }
        }

        auto err = GetCentral(this->central_heap_storage)->Free(ptr);
        AMS_ASSERT(err == 0);
    }

    void *StandardAllocator::Reallocate(void *ptr, size_t new_size) {
        AMS_ASSERT(this->initialized);

        if (new_size > impl::MaxSize) {
            return nullptr;
        }

        if (ptr == nullptr) {
            return this->Allocate(new_size);
        }

        if (new_size == 0) {
            this->Free(ptr);
            return nullptr;
        }

        size_t aligned_new_size = util::AlignUp(new_size, DefaultAlignment);


        impl::heap::TlsHeapCache *heap_cache = nullptr;
        if (this->enable_thread_cache) {
            heap_cache = reinterpret_cast<impl::heap::TlsHeapCache *>(os::GetTlsValue(this->tls_slot));
            if (!heap_cache) {
                GetCache(GetCentral(this->central_heap_storage), this->tls_slot);
                heap_cache = reinterpret_cast<impl::heap::TlsHeapCache *>(os::GetTlsValue(this->tls_slot));
            }
        }

        void *p = nullptr;
        impl::errno_t err;
        if (heap_cache) {
            err = heap_cache->Reallocate(ptr, aligned_new_size, std::addressof(p));
        } else {
            err = GetCentral(this->central_heap_storage)->Reallocate(ptr, aligned_new_size, std::addressof(p));
        }

        if (err == 0) {
            return p;
        } else {
            return nullptr;
        }
    }

    size_t StandardAllocator::Shrink(void *ptr, size_t new_size) {
        AMS_ASSERT(this->initialized);

        if (this->enable_thread_cache) {
            impl::heap::TlsHeapCache *heap_cache = reinterpret_cast<impl::heap::TlsHeapCache *>(os::GetTlsValue(this->tls_slot));
            if (heap_cache) {
                if (heap_cache->Shrink(ptr, new_size) == 0) {
                    return heap_cache->GetAllocationSize(ptr);
                } else {
                    return 0;
                }
            }
        }

        if (GetCentral(this->central_heap_storage)->Shrink(ptr, new_size) == 0) {
            return GetCentral(this->central_heap_storage)->GetAllocationSize(ptr);
        } else {
            return 0;
        }
    }

    void StandardAllocator::ClearThreadCache() const {
        if (this->enable_thread_cache) {
            impl::heap::TlsHeapCache *heap_cache = reinterpret_cast<impl::heap::TlsHeapCache *>(os::GetTlsValue(this->tls_slot));
            impl::heap::CachedHeap cache;
            cache.Reset(heap_cache);
            cache.Query(impl::AllocQuery_ClearCache);
            cache.Release();
        }
    }

    void StandardAllocator::CleanUpManagementArea() const {
        AMS_ASSERT(this->initialized);

        auto err = GetCentral(this->central_heap_storage)->Query(impl::AllocQuery_UnifyFreeList);
        AMS_ASSERT(err == 0);
    }

    size_t StandardAllocator::GetSizeOf(const void *ptr) const {
        AMS_ASSERT(this->initialized);

        if (!util::IsAligned(reinterpret_cast<uintptr_t>(ptr), DefaultAlignment)) {
            return 0;
        }

        impl::heap::TlsHeapCache *heap_cache = nullptr;
        if (this->enable_thread_cache) {
            heap_cache = reinterpret_cast<impl::heap::TlsHeapCache *>(os::GetTlsValue(this->tls_slot));
            if (!heap_cache) {
                GetCache(GetCentral(this->central_heap_storage), this->tls_slot);
                heap_cache = reinterpret_cast<impl::heap::TlsHeapCache *>(os::GetTlsValue(this->tls_slot));
            }
        }

        if (heap_cache) {
            return heap_cache->GetAllocationSize(ptr);
        } else {
            return GetCentral(this->central_heap_storage)->GetAllocationSize(ptr);
        }
    }

    size_t StandardAllocator::GetTotalFreeSize() const {
        size_t size = 0;

        auto err = GetCentral(this->central_heap_storage)->Query(impl::AllocQuery_FreeSizeMapped, std::addressof(size));
        if (err != 0) {
            err = GetCentral(this->central_heap_storage)->Query(impl::AllocQuery_FreeSize, std::addressof(size));
        }
        AMS_ASSERT(err == 0);

        return size;
    }

    size_t StandardAllocator::GetAllocatableSize() const {
        size_t size = 0;

        auto err = GetCentral(this->central_heap_storage)->Query(impl::AllocQuery_MaxAllocatableSizeMapped, std::addressof(size));
        if (err != 0) {
            err = GetCentral(this->central_heap_storage)->Query(impl::AllocQuery_MaxAllocatableSize, std::addressof(size));
        }
        AMS_ASSERT(err == 0);

        return size;
    }

    void StandardAllocator::WalkAllocatedBlocks(WalkCallback callback, void *user_data) const {
        AMS_ASSERT(this->initialized);
        this->ClearThreadCache();
        GetCentral(this->central_heap_storage)->WalkAllocatedPointers(callback, user_data);
    }

    void StandardAllocator::Dump() const {
        AMS_ASSERT(this->initialized);

        size_t tmp;
        auto err = GetCentral(this->central_heap_storage)->Query(impl::AllocQuery_MaxAllocatableSizeMapped, std::addressof(tmp));

        if (err == 0) {
            GetCentral(this->central_heap_storage)->Query(impl::AllocQuery_Dump, impl::DumpMode_Spans | impl::DumpMode_Pointers, 1);
        } else {
            GetCentral(this->central_heap_storage)->Query(impl::AllocQuery_Dump, impl::DumpMode_All, 1);
        }
    }

    StandardAllocator::AllocatorHash StandardAllocator::Hash() const {
        AMS_ASSERT(this->initialized);

        AllocatorHash alloc_hash;
        {
            char temp_hash[crypto::Sha1Generator::HashSize];
            InternalHash internal_hash;
            internal_hash.allocated_count = 0;
            internal_hash.allocated_size  = 0;
            internal_hash.sha1.Initialize();

            this->WalkAllocatedBlocks(InternalHashCallback, reinterpret_cast<void *>(std::addressof(internal_hash)));

            alloc_hash.allocated_count = internal_hash.allocated_count;
            alloc_hash.allocated_size  = internal_hash.allocated_size;

            internal_hash.sha1.GetHash(temp_hash, sizeof(temp_hash));
            std::memcpy(std::addressof(alloc_hash.hash), temp_hash, sizeof(alloc_hash.hash));
        }
        return alloc_hash;
    }


}
