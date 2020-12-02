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

namespace ams::fssystem::save {

    namespace {

        constexpr inline uintptr_t InvalidAddress = 0;
        constexpr inline s64 InvalidOffset = std::numeric_limits<s64>::max();

    }

    class BufferedStorage::Cache : public ::ams::fs::impl::Newable {
        private:
            struct FetchParameter {
                s64 offset;
                void *buffer;
                size_t size;
            };
            static_assert(util::is_pod<FetchParameter>::value);
        private:
            BufferedStorage *buffered_storage;
            std::pair<uintptr_t, size_t> memory_range;
            IBufferManager::CacheHandle cache_handle;
            s64 offset;
            std::atomic<bool> is_valid;
            std::atomic<bool> is_dirty;
            u8 reserved[2];
            s32 reference_count;
            Cache *next;
            Cache *prev;
        public:
            Cache() : buffered_storage(nullptr), memory_range(InvalidAddress, 0), cache_handle(), offset(InvalidOffset), is_valid(false), is_dirty(false), reference_count(1), next(nullptr), prev(nullptr) {
                /* ... */
            }

            ~Cache() {
                this->Finalize();
            }

            void Initialize(BufferedStorage *bs) {
                AMS_ASSERT(bs != nullptr);
                AMS_ASSERT(this->buffered_storage == nullptr);

                this->buffered_storage = bs;
                this->Link();
            }

            void Finalize() {
                AMS_ASSERT(this->buffered_storage != nullptr);
                AMS_ASSERT(this->buffered_storage->buffer_manager != nullptr);
                AMS_ASSERT(this->reference_count == 0);

                /* If we're valid, acquire our cache handle and free our buffer. */
                if (this->IsValid()) {
                    const auto buffer_manager = this->buffered_storage->buffer_manager;
                    if (!this->is_dirty) {
                        AMS_ASSERT(this->memory_range.first == InvalidAddress);
                        this->memory_range = buffer_manager->AcquireCache(this->cache_handle);
                    }
                    if (this->memory_range.first != InvalidAddress) {
                        buffer_manager->DeallocateBuffer(this->memory_range.first, this->memory_range.second);
                        this->memory_range.first  = InvalidAddress;
                        this->memory_range.second = 0;
                    }
                }

                /* Clear all our members. */
                this->buffered_storage = nullptr;
                this->offset           = InvalidOffset;
                this->is_valid         = false;
                this->is_dirty         = false;
                this->next             = nullptr;
                this->prev             = nullptr;
            }

            void Link() {
                AMS_ASSERT(this->buffered_storage != nullptr);
                AMS_ASSERT(this->buffered_storage->buffer_manager != nullptr);
                AMS_ASSERT(this->reference_count > 0);

                if ((--this->reference_count) == 0) {
                    AMS_ASSERT(this->next == nullptr);
                    AMS_ASSERT(this->prev == nullptr);

                    if (this->buffered_storage->next_fetch_cache == nullptr) {
                        this->buffered_storage->next_fetch_cache = this;
                        this->next = this;
                        this->prev = this;
                    } else {
                        /* Check against a cache being registered twice. */
                        {
                            auto cache = this->buffered_storage->next_fetch_cache;
                            do {
                                if (cache->IsValid() && this->Hits(cache->offset, this->buffered_storage->block_size)) {
                                    this->is_valid = false;
                                    break;
                                }
                                cache = cache->next;
                            } while (cache != this->buffered_storage->next_fetch_cache);
                        }

                        /* Link into the fetch list. */
                        {
                            AMS_ASSERT(this->buffered_storage->next_fetch_cache->prev != nullptr);
                            AMS_ASSERT(this->buffered_storage->next_fetch_cache->prev->next == this->buffered_storage->next_fetch_cache);
                            this->next = this->buffered_storage->next_fetch_cache;
                            this->prev = this->buffered_storage->next_fetch_cache->prev;
                            this->next->prev = this;
                            this->prev->next = this;
                        }

                        /* Insert invalid caches at the start of the list. */
                        if (!this->IsValid()) {
                            this->buffered_storage->next_fetch_cache = this;
                        }
                    }

                    /* If we're not valid, clear our offset. */
                    if (!this->IsValid()) {
                        this->offset = InvalidOffset;
                        this->is_dirty = false;
                    }

                    /* Ensure our buffer state is coherent. */
                    if (this->memory_range.first != InvalidAddress && !this->is_dirty) {
                        if (this->IsValid()) {
                            this->cache_handle = this->buffered_storage->buffer_manager->RegisterCache(this->memory_range.first, this->memory_range.second, IBufferManager::BufferAttribute());
                        } else {
                            this->buffered_storage->buffer_manager->DeallocateBuffer(this->memory_range.first, this->memory_range.second);
                        }
                        this->memory_range.first = InvalidAddress;
                        this->memory_range.second = 0;
                    }
                }
            }

            void Unlink() {
                AMS_ASSERT(this->buffered_storage != nullptr);
                AMS_ASSERT(this->reference_count >= 0);

                if ((++this->reference_count) == 1) {
                    AMS_ASSERT(this->next != nullptr);
                    AMS_ASSERT(this->prev != nullptr);
                    AMS_ASSERT(this->next->prev == this);
                    AMS_ASSERT(this->prev->next == this);

                    if (this->buffered_storage->next_fetch_cache == this) {
                        if (this->next != this) {
                            this->buffered_storage->next_fetch_cache = this->next;
                        } else {
                            this->buffered_storage->next_fetch_cache = nullptr;
                        }
                    }

                    this->buffered_storage->next_acquire_cache = this;

                    this->next->prev = this->prev;
                    this->prev->next = this->next;
                    this->next = nullptr;
                    this->prev = nullptr;
                } else {
                    AMS_ASSERT(this->next == nullptr);
                    AMS_ASSERT(this->prev == nullptr);
                }
            }

            void Read(s64 offset, void *buffer, size_t size) const {
                AMS_ASSERT(this->buffered_storage != nullptr);
                AMS_ASSERT(this->next == nullptr);
                AMS_ASSERT(this->prev == nullptr);
                AMS_ASSERT(this->IsValid());
                AMS_ASSERT(this->Hits(offset, 1));
                AMS_ASSERT(this->memory_range.first != InvalidAddress);

                const auto read_offset         = offset - this->offset;
                const auto readable_offset_max = this->buffered_storage->block_size - size;
                const auto cache_buffer        = reinterpret_cast<u8 *>(this->memory_range.first) + read_offset;
                AMS_ASSERT(read_offset >= 0);
                AMS_ASSERT(static_cast<size_t>(read_offset) <= readable_offset_max);

                std::memcpy(buffer, cache_buffer, size);
            }

            void Write(s64 offset, const void *buffer, size_t size) {
                AMS_ASSERT(this->buffered_storage != nullptr);
                AMS_ASSERT(this->next == nullptr);
                AMS_ASSERT(this->prev == nullptr);
                AMS_ASSERT(this->IsValid());
                AMS_ASSERT(this->Hits(offset, 1));
                AMS_ASSERT(this->memory_range.first != InvalidAddress);

                const auto write_offset        = offset - this->offset;
                const auto writable_offset_max = this->buffered_storage->block_size - size;
                const auto cache_buffer        = reinterpret_cast<u8 *>(this->memory_range.first) + write_offset;
                AMS_ASSERT(write_offset >= 0);
                AMS_ASSERT(static_cast<size_t>(write_offset) <= writable_offset_max);

                std::memcpy(cache_buffer, buffer, size);
                this->is_dirty = true;
            }

            Result Flush() {
                AMS_ASSERT(this->buffered_storage != nullptr);
                AMS_ASSERT(this->next == nullptr);
                AMS_ASSERT(this->prev == nullptr);
                AMS_ASSERT(this->IsValid());

                if (this->is_dirty) {
                    AMS_ASSERT(this->memory_range.first != InvalidAddress);

                    const auto base_size  = this->buffered_storage->base_storage_size;
                    const auto block_size = static_cast<s64>(this->buffered_storage->block_size);
                    const auto flush_size = static_cast<size_t>(std::min(block_size, base_size - this->offset));

                    auto &base_storage = this->buffered_storage->base_storage;
                    const auto cache_buffer = reinterpret_cast<void *>(this->memory_range.first);

                    R_TRY(base_storage.Write(this->offset, cache_buffer, flush_size));
                    this->is_dirty = false;

                    buffers::EnableBlockingBufferManagerAllocation();
                }

                return ResultSuccess();
            }

            const std::pair<Result, bool> PrepareFetch() {
                AMS_ASSERT(this->buffered_storage != nullptr);
                AMS_ASSERT(this->buffered_storage->buffer_manager != nullptr);
                AMS_ASSERT(this->next == nullptr);
                AMS_ASSERT(this->prev == nullptr);
                AMS_ASSERT(this->IsValid());
                AMS_ASSERT(this->buffered_storage->mutex.IsLockedByCurrentThread());

                std::pair<Result, bool> result(ResultSuccess(), false);
                if (this->reference_count == 1) {
                    result.first = this->Flush();
                    if (R_SUCCEEDED(result.first)) {
                        this->is_valid = false;
                        this->reference_count = 0;
                        result.second = true;
                    }
                }

                return result;
            }

            void UnprepareFetch() {
                AMS_ASSERT(this->buffered_storage != nullptr);
                AMS_ASSERT(this->buffered_storage->buffer_manager != nullptr);
                AMS_ASSERT(this->next == nullptr);
                AMS_ASSERT(this->prev == nullptr);
                AMS_ASSERT(!this->IsValid());
                AMS_ASSERT(!this->is_dirty);
                AMS_ASSERT(this->buffered_storage->mutex.IsLockedByCurrentThread());

                this->is_valid = true;
                this->reference_count = 1;
            }

            Result Fetch(s64 offset) {
                AMS_ASSERT(this->buffered_storage != nullptr);
                AMS_ASSERT(this->buffered_storage->buffer_manager != nullptr);
                AMS_ASSERT(this->next == nullptr);
                AMS_ASSERT(this->prev == nullptr);
                AMS_ASSERT(!this->IsValid());
                AMS_ASSERT(!this->is_dirty);

                if (this->memory_range.first == InvalidAddress) {
                    R_TRY(this->AllocateFetchBuffer());
                }

                FetchParameter fetch_param = {};
                this->CalcFetchParameter(std::addressof(fetch_param), offset);

                auto &base_storage = this->buffered_storage->base_storage;
                R_TRY(base_storage.Read(fetch_param.offset, fetch_param.buffer, fetch_param.size));
                this->offset = fetch_param.offset;
                AMS_ASSERT(this->Hits(offset, 1));

                return ResultSuccess();
            }

            Result FetchFromBuffer(s64 offset, const void *buffer, size_t buffer_size) {
                AMS_ASSERT(this->buffered_storage != nullptr);
                AMS_ASSERT(this->buffered_storage->buffer_manager != nullptr);
                AMS_ASSERT(this->next == nullptr);
                AMS_ASSERT(this->prev == nullptr);
                AMS_ASSERT(!this->IsValid());
                AMS_ASSERT(!this->is_dirty);
                AMS_ASSERT(util::IsAligned(offset, this->buffered_storage->block_size));

                if (this->memory_range.first == InvalidAddress) {
                    R_TRY(this->AllocateFetchBuffer());
                }

                FetchParameter fetch_param = {};
                this->CalcFetchParameter(std::addressof(fetch_param), offset);
                AMS_ASSERT(fetch_param.offset == offset);
                AMS_ASSERT(fetch_param.size <= buffer_size);

                std::memcpy(fetch_param.buffer, buffer, fetch_param.size);
                this->offset = fetch_param.offset;
                AMS_ASSERT(this->Hits(offset, 1));

                return ResultSuccess();
            }

            bool TryAcquireCache() {
                AMS_ASSERT(this->buffered_storage != nullptr);
                AMS_ASSERT(this->buffered_storage->buffer_manager != nullptr);
                AMS_ASSERT(this->IsValid());

                if (this->memory_range.first != InvalidAddress) {
                    return true;
                } else {
                    this->memory_range = this->buffered_storage->buffer_manager->AcquireCache(this->cache_handle);
                    this->is_valid = this->memory_range.first != InvalidAddress;
                    return this->is_valid;
                }
            }

            void Invalidate() {
                AMS_ASSERT(this->buffered_storage != nullptr);
                this->is_valid = false;
            }

            bool IsValid() const {
                AMS_ASSERT(this->buffered_storage != nullptr);
                return this->is_valid || this->reference_count > 0;
            }

            bool IsDirty() const {
                AMS_ASSERT(this->buffered_storage != nullptr);
                return this->is_dirty;
            }

            bool Hits(s64 offset, s64 size) const {
                AMS_ASSERT(this->buffered_storage != nullptr);
                const auto block_size = static_cast<s64>(this->buffered_storage->block_size);
                return (offset < this->offset + block_size) && (this->offset < offset + size);
            }
        private:
            Result AllocateFetchBuffer() {
                IBufferManager *buffer_manager = this->buffered_storage->buffer_manager;
                AMS_ASSERT(buffer_manager->AcquireCache(this->cache_handle).first == InvalidAddress);

                auto range_guard = SCOPE_GUARD { this->memory_range.first = InvalidAddress; };
                R_TRY(buffers::AllocateBufferUsingBufferManagerContext(std::addressof(this->memory_range), buffer_manager, this->buffered_storage->block_size, IBufferManager::BufferAttribute(), [](const std::pair<uintptr_t, size_t> &buffer) {
                    return buffer.first != 0;
                }, AMS_CURRENT_FUNCTION_NAME));

                range_guard.Cancel();
                return ResultSuccess();
            }

            void CalcFetchParameter(FetchParameter *out, s64 offset) const {
                AMS_ASSERT(out != nullptr);

                const auto block_size   = static_cast<s64>(this->buffered_storage->block_size);
                const auto cache_offset = util::AlignDown(offset, this->buffered_storage->block_size);
                const auto base_size    = this->buffered_storage->base_storage_size;
                const auto cache_size   = static_cast<size_t>(std::min(block_size, base_size - cache_offset));
                const auto cache_buffer = reinterpret_cast<void *>(this->memory_range.first);
                AMS_ASSERT(offset >= 0);
                AMS_ASSERT(offset < base_size);

                out->offset = cache_offset;
                out->buffer = cache_buffer;
                out->size   = cache_size;
            }
    };

    class BufferedStorage::SharedCache {
        NON_COPYABLE(SharedCache);
        NON_MOVEABLE(SharedCache);
        friend class UniqueCache;
        private:
            Cache *cache;
            Cache *start_cache;
            BufferedStorage *buffered_storage;
        public:
            explicit SharedCache(BufferedStorage *bs) : cache(nullptr), start_cache(bs->next_acquire_cache), buffered_storage(bs) {
                AMS_ASSERT(this->buffered_storage != nullptr);
            }

            ~SharedCache() {
                std::scoped_lock lk(buffered_storage->mutex);
                this->Release();
            }

            bool AcquireNextOverlappedCache(s64 offset, s64 size) {
                AMS_ASSERT(this->buffered_storage != nullptr);

                auto is_first = this->cache == nullptr;
                const auto start    = is_first ? this->start_cache : this->cache + 1;

                AMS_ASSERT(start >= this->buffered_storage->caches.get());
                AMS_ASSERT(start <= this->buffered_storage->caches.get() + this->buffered_storage->cache_count);

                std::scoped_lock lk(this->buffered_storage->mutex);

                this->Release();
                AMS_ASSERT(this->cache == nullptr);

                for (auto cache = start; true; ++cache) {
                    if (this->buffered_storage->caches.get() + this->buffered_storage->cache_count <= cache) {
                        cache = this->buffered_storage->caches.get();
                    }
                    if (!is_first && cache == this->start_cache) {
                        break;
                    }
                    if (cache->IsValid() && cache->Hits(offset, size) && cache->TryAcquireCache()) {
                        cache->Unlink();
                        this->cache = cache;
                        return true;
                    }
                    is_first = false;
                }

                this->cache = nullptr;
                return false;
            }

            bool AcquireNextDirtyCache() {
                AMS_ASSERT(this->buffered_storage != nullptr);
                const auto start = this->cache != nullptr ? this->cache + 1 : this->buffered_storage->caches.get();
                const auto end   = this->buffered_storage->caches.get() + this->buffered_storage->cache_count;

                AMS_ASSERT(start >= this->buffered_storage->caches.get());
                AMS_ASSERT(start <= end);

                this->Release();
                AMS_ASSERT(this->cache == nullptr);

                for (auto cache = start; cache < end; ++cache) {
                    if (cache->IsValid() && cache->IsDirty() && cache->TryAcquireCache()) {
                        cache->Unlink();
                        this->cache = cache;
                        return true;
                    }
                }

                this->cache = nullptr;
                return false;
            }

            bool AcquireNextValidCache() {
                AMS_ASSERT(this->buffered_storage != nullptr);
                const auto start = this->cache != nullptr ? this->cache + 1 : this->buffered_storage->caches.get();
                const auto end   = this->buffered_storage->caches.get() + this->buffered_storage->cache_count;

                AMS_ASSERT(start >= this->buffered_storage->caches.get());
                AMS_ASSERT(start <= end);

                this->Release();
                AMS_ASSERT(this->cache == nullptr);

                for (auto cache = start; cache < end; ++cache) {
                    if (cache->IsValid() && cache->TryAcquireCache()) {
                        cache->Unlink();
                        this->cache = cache;
                        return true;
                    }
                }

                this->cache = nullptr;
                return false;
            }

            bool AcquireFetchableCache() {
                AMS_ASSERT(this->buffered_storage != nullptr);

                std::scoped_lock lk(this->buffered_storage->mutex);

                this->Release();
                AMS_ASSERT(this->cache == nullptr);

                this->cache = this->buffered_storage->next_fetch_cache;
                if (this->cache != nullptr) {
                    if (this->cache->IsValid()) {
                        this->cache->TryAcquireCache();
                    }
                    this->cache->Unlink();
                }

                return this->cache != nullptr;
            }

            void Read(s64 offset, void *buffer, size_t size) {
                AMS_ASSERT(this->cache != nullptr);
                this->cache->Read(offset, buffer, size);
            }

            void Write(s64 offset, const void *buffer, size_t size) {
                AMS_ASSERT(this->cache != nullptr);
                this->cache->Write(offset, buffer, size);
            }

            Result Flush() {
                AMS_ASSERT(this->cache != nullptr);
                return this->cache->Flush();
            }

            void Invalidate() {
                AMS_ASSERT(this->cache != nullptr);
                return this->cache->Invalidate();
            }

            bool Hits(s64 offset, s64 size) const {
                AMS_ASSERT(this->cache != nullptr);
                return this->cache->Hits(offset, size);
            }
        private:
            void Release() {
                if (this->cache != nullptr) {
                    AMS_ASSERT(this->buffered_storage->caches.get() <= this->cache);
                    AMS_ASSERT(this->cache <= this->buffered_storage->caches.get() + this->buffered_storage->cache_count);

                    this->cache->Link();
                    this->cache = nullptr;
                }
            }
    };

    class BufferedStorage::UniqueCache {
        NON_COPYABLE(UniqueCache);
        NON_MOVEABLE(UniqueCache);
        private:
            Cache *cache;
            BufferedStorage *buffered_storage;
        public:
            explicit UniqueCache(BufferedStorage *bs) : cache(nullptr), buffered_storage(bs) {
                AMS_ASSERT(this->buffered_storage != nullptr);
            }

            ~UniqueCache() {
                if (this->cache != nullptr) {
                    std::scoped_lock lk(this->buffered_storage->mutex);
                    this->cache->UnprepareFetch();
                }
            }

            const std::pair<Result, bool> Upgrade(const SharedCache &shared_cache) {
                AMS_ASSERT(this->buffered_storage == shared_cache.buffered_storage);
                AMS_ASSERT(shared_cache.cache != nullptr);

                std::scoped_lock lk(this->buffered_storage->mutex);
                const auto result = shared_cache.cache->PrepareFetch();
                if (R_SUCCEEDED(result.first) && result.second) {
                    this->cache = shared_cache.cache;
                }
                return result;
            }

            Result Fetch(s64 offset) {
                AMS_ASSERT(this->cache != nullptr);
                return this->cache->Fetch(offset);
            }

            Result FetchFromBuffer(s64 offset, const void *buffer, size_t buffer_size) {
                AMS_ASSERT(this->cache != nullptr);
                R_TRY(this->cache->FetchFromBuffer(offset, buffer, buffer_size));
                return ResultSuccess();
            }
    };

    BufferedStorage::BufferedStorage() : base_storage(), buffer_manager(), block_size(), base_storage_size(), caches(), cache_count(), next_acquire_cache(), next_fetch_cache(), mutex(false), bulk_read_enabled() {
        /* ... */
    }

    BufferedStorage::~BufferedStorage() {
       this->Finalize();
    }

    Result BufferedStorage::Initialize(fs::SubStorage base_storage, IBufferManager *buffer_manager, size_t block_size, s32 buffer_count) {
        AMS_ASSERT(buffer_manager != nullptr);
        AMS_ASSERT(block_size > 0);
        AMS_ASSERT(util::IsPowerOfTwo(block_size));
        AMS_ASSERT(buffer_count > 0);

        /* Get the base storage size. */
        R_TRY(base_storage.GetSize(std::addressof(this->base_storage_size)));

        /* Set members. */
        this->base_storage   = base_storage;
        this->buffer_manager = buffer_manager;
        this->block_size     = block_size;
        this->cache_count    = buffer_count;

        /* Allocate the caches. */
        this->caches.reset(new Cache[buffer_count]);
        R_UNLESS(this->caches != nullptr, fs::ResultAllocationFailureInBufferedStorageA());

        /* Initialize the caches. */
        for (auto i = 0; i < buffer_count; i++) {
            this->caches[i].Initialize(this);
        }

        this->next_acquire_cache = std::addressof(this->caches[0]);
        return ResultSuccess();
    }

    void BufferedStorage::Finalize() {
        this->base_storage = fs::SubStorage();
        this->base_storage_size = 0;
        this->caches.reset();
        this->cache_count = 0;
        this->next_fetch_cache = nullptr;
    }

    Result BufferedStorage::Read(s64 offset, void *buffer, size_t size) {
        AMS_ASSERT(this->IsInitialized());

        /* Succeed if zero size. */
        R_SUCCEED_IF(size == 0);

        /* Validate arguments. */
        R_UNLESS(buffer != nullptr, fs::ResultNullptrArgument());

        /* Do the read. */
        R_TRY(this->ReadCore(offset, buffer, size));
        return ResultSuccess();
    }

    Result BufferedStorage::Write(s64 offset, const void *buffer, size_t size) {
        AMS_ASSERT(this->IsInitialized());

        /* Succeed if zero size. */
        R_SUCCEED_IF(size == 0);

        /* Validate arguments. */
        R_UNLESS(buffer != nullptr, fs::ResultNullptrArgument());

        /* Do the write. */
        R_TRY(this->WriteCore(offset, buffer, size));
        return ResultSuccess();
    }

    Result BufferedStorage::GetSize(s64 *out) {
        AMS_ASSERT(out != nullptr);
        AMS_ASSERT(this->IsInitialized());

        *out = this->base_storage_size;
        return ResultSuccess();
    }

    Result BufferedStorage::SetSize(s64 size) {
        AMS_ASSERT(this->IsInitialized());
        const s64 prev_size = this->base_storage_size;
        if (prev_size < size) {
            /* Prepare to expand. */
            if (!util::IsAligned(prev_size, this->block_size)) {
                SharedCache cache(this);
                const auto invalidate_offset = prev_size;
                const auto invalidate_size   = size - prev_size;
                if (cache.AcquireNextOverlappedCache(invalidate_offset, invalidate_size)) {
                    R_TRY(cache.Flush());
                    cache.Invalidate();
                }
                /* AMS_ASSERT(!cache.AcquireNextOverlappedCache(invalidate_offset, invalidate_size)); */
            }
        } else if (size < prev_size) {
            /* Prepare to do a shrink. */
            SharedCache cache(this);
            const auto invalidate_offset = prev_size;
            const auto invalidate_size   = size - prev_size;
            const auto is_fragment       = util::IsAligned(size, this->block_size);
            while (cache.AcquireNextOverlappedCache(invalidate_offset, invalidate_size)) {
                if (is_fragment && cache.Hits(invalidate_offset, 1)) {
                    R_TRY(cache.Flush());
                }
                cache.Invalidate();
            }
        }

        /* Set the size. */
        R_TRY(this->base_storage.SetSize(size));

        /* Get our new size. */
        s64 new_size = 0;
        R_TRY(this->base_storage.GetSize(std::addressof(new_size)));

        this->base_storage_size = new_size;
        return ResultSuccess();
    }

    Result BufferedStorage::Flush()  {
        AMS_ASSERT(this->IsInitialized());

        /* Flush caches. */
        SharedCache cache(this);
        while (cache.AcquireNextDirtyCache()) {
            R_TRY(cache.Flush());
        }

        /* Flush the base storage. */
        R_TRY(this->base_storage.Flush());
        return ResultSuccess();
    }

    Result BufferedStorage::OperateRange(void *dst, size_t dst_size, fs::OperationId op_id, s64 offset, s64 size, const void *src, size_t src_size)  {
        AMS_ASSERT(this->IsInitialized());

        /* Invalidate caches, if we should. */
        if (op_id == fs::OperationId::InvalidateCache) {
            SharedCache cache(this);
            while (cache.AcquireNextOverlappedCache(offset, size)) {
                cache.Invalidate();
            }
        }

        return this->base_storage.OperateRange(dst, dst_size, op_id, offset, size, src, src_size);
    }

    void BufferedStorage::InvalidateCaches() {
        AMS_ASSERT(this->IsInitialized());

        SharedCache cache(this);
        while (cache.AcquireNextValidCache()) {
            cache.Invalidate();
        }
    }

    Result BufferedStorage::PrepareAllocation() {
        const auto flush_threshold = this->buffer_manager->GetTotalSize() / 8;
        if (this->buffer_manager->GetTotalAllocatableSize() < flush_threshold) {
            R_TRY(this->Flush());
        }
        return ResultSuccess();
    }

    Result BufferedStorage::ControlDirtiness() {
        const auto flush_threshold = this->buffer_manager->GetTotalSize() / 4;
        if (this->buffer_manager->GetTotalAllocatableSize() < flush_threshold) {
            s32 dirty_count = 0;
            SharedCache cache(this);
            while (cache.AcquireNextDirtyCache()) {
                if ((++dirty_count) > 1) {
                    R_TRY(cache.Flush());
                    cache.Invalidate();
                }
            }
        }
        return ResultSuccess();
    }

    Result BufferedStorage::ReadCore(s64 offset, void *buffer, size_t size) {
        AMS_ASSERT(this->caches != nullptr);
        AMS_ASSERT(buffer != nullptr);

        /* Validate the offset. */
        const auto base_storage_size = this->base_storage_size;
        R_UNLESS(offset >= 0,                 fs::ResultInvalidOffset());
        R_UNLESS(offset <= base_storage_size, fs::ResultInvalidOffset());

        /* Setup tracking variables. */
        size_t remaining_size = static_cast<size_t>(std::min<s64>(size, base_storage_size - offset));
        s64 cur_offset        = offset;
        s64 buf_offset        = 0;

        /* Determine what caches are needed, if we have bulk read set. */
        if (this->bulk_read_enabled) {
            /* Check head cache. */
            const auto head_cache_needed = this->ReadHeadCache(std::addressof(cur_offset), buffer, std::addressof(remaining_size), std::addressof(buf_offset));
            R_SUCCEED_IF(remaining_size == 0);

            /* Check tail cache. */
            const auto tail_cache_needed = this->ReadTailCache(cur_offset, buffer, std::addressof(remaining_size), buf_offset);
            R_SUCCEED_IF(remaining_size == 0);

            /* Perform bulk reads. */
            constexpr size_t BulkReadSizeMax = 2_MB;
            if (remaining_size <= BulkReadSizeMax) {
                do {
                    /* Try to do a bulk read. */
                    R_TRY_CATCH(this->BulkRead(cur_offset, static_cast<u8 *>(buffer) + buf_offset, remaining_size, head_cache_needed, tail_cache_needed)) {
                        R_CATCH(fs::ResultAllocationFailurePooledBufferNotEnoughSize) {
                            /* If the read fails due to insufficient pooled buffer size, */
                            /* then we want to fall back to the normal read path. */
                            break;
                        }
                    } R_END_TRY_CATCH;

                    return ResultSuccess();
                } while(0);
            }
        }

        /* Repeatedly read until we're done. */
        while (remaining_size > 0) {
            /* Determine how much to read this iteration. */
            auto *cur_dst = static_cast<u8 *>(buffer) + buf_offset;
            size_t cur_size = 0;

            if (!util::IsAligned(cur_offset, this->block_size)) {
                const size_t aligned_size = this->block_size - (cur_offset & (this->block_size - 1));
                cur_size = std::min(aligned_size, remaining_size);
            } else if (remaining_size < this->block_size) {
                cur_size = remaining_size;
            } else {
                cur_size = util::AlignDown(remaining_size, this->block_size);
            }

            if (cur_size <= this->block_size) {
                SharedCache cache(this);
                if (!cache.AcquireNextOverlappedCache(cur_offset, cur_size)) {
                    R_TRY(this->PrepareAllocation());
                    while (true) {
                        R_UNLESS(cache.AcquireFetchableCache(), fs::ResultOutOfResource());

                        UniqueCache fetch_cache(this);
                        const auto upgrade_result = fetch_cache.Upgrade(cache);
                        R_TRY(upgrade_result.first);
                        if (upgrade_result.second) {
                            R_TRY(fetch_cache.Fetch(cur_offset));
                            break;
                        }
                    }
                    R_TRY(this->ControlDirtiness());
                }
                cache.Read(cur_offset, cur_dst, cur_size);
            } else {
                {
                    SharedCache cache(this);
                    while (cache.AcquireNextOverlappedCache(cur_offset, cur_size)) {
                        R_TRY(cache.Flush());
                        cache.Invalidate();
                    }
                }
                R_TRY(this->base_storage.Read(cur_offset, cur_dst, cur_size));
            }

            remaining_size -= cur_size;
            cur_offset     += cur_size;
            buf_offset     += cur_size;
        }

        return ResultSuccess();
    }

    bool BufferedStorage::ReadHeadCache(s64 *offset, void *buffer, size_t *size, s64 *buffer_offset) {
        AMS_ASSERT(offset        != nullptr);
        AMS_ASSERT(buffer        != nullptr);
        AMS_ASSERT(size          != nullptr);
        AMS_ASSERT(buffer_offset != nullptr);

        bool is_cache_needed = !util::IsAligned(*offset, this->block_size);

        while (*size > 0) {
            size_t cur_size = 0;

            if (!util::IsAligned(*offset, this->block_size)) {
                const s64 aligned_size = util::AlignUp(*offset, this->block_size) - *offset;
                cur_size = std::min(aligned_size, static_cast<s64>(*size));
            } else if (*size < this->block_size) {
                cur_size = *size;
            } else {
                cur_size = this->block_size;
            }

            SharedCache cache(this);
            if (!cache.AcquireNextOverlappedCache(*offset, cur_size)) {
                break;
            }

            cache.Read(*offset, static_cast<u8 *>(buffer) + *buffer_offset, cur_size);
            *offset        += cur_size;
            *buffer_offset += cur_size;
            *size          -= cur_size;
            is_cache_needed = false;
        }

        return is_cache_needed;
    }

    bool BufferedStorage::ReadTailCache(s64 offset, void *buffer, size_t *size, s64 buffer_offset) {
        AMS_ASSERT(buffer        != nullptr);
        AMS_ASSERT(size          != nullptr);

        bool is_cache_needed = !util::IsAligned(offset + *size, this->block_size);

        while (*size > 0) {
            const s64 cur_offset_end = offset + *size;
            size_t cur_size = 0;

            if (!util::IsAligned(offset, this->block_size)) {
                const s64 aligned_size = cur_offset_end - util::AlignDown(cur_offset_end, this->block_size);
                cur_size = std::min(aligned_size, static_cast<s64>(*size));
            } else if (*size < this->block_size) {
                cur_size = *size;
            } else {
                cur_size = this->block_size;
            }

            const s64 cur_offset = cur_offset_end - static_cast<s64>(cur_size);
            AMS_ASSERT(cur_offset >= 0);

            SharedCache cache(this);
            if (!cache.AcquireNextOverlappedCache(cur_offset, cur_size)) {
                break;
            }

            cache.Read(cur_offset, static_cast<u8 *>(buffer) + buffer_offset + cur_offset - offset, cur_size);
            *size          -= cur_size;
            is_cache_needed = false;
        }

        return is_cache_needed;
    }

    Result BufferedStorage::BulkRead(s64 offset, void *buffer, size_t size, bool head_cache_needed, bool tail_cache_needed) {
        /* Determine aligned extents. */
        const s64 aligned_offset     = util::AlignDown(offset, this->block_size);
        const s64 aligned_offset_end = std::min(util::AlignUp(offset + static_cast<s64>(size), this->block_size), this->base_storage_size);
        const s64 aligned_size       = aligned_offset_end - aligned_offset;

        /* Allocate a work buffer. */
        char *work_buffer = nullptr;
        PooledBuffer pooled_buffer;
        if (offset == aligned_offset && size == static_cast<size_t>(aligned_size)) {
            work_buffer = static_cast<char *>(buffer);
        } else {
            pooled_buffer.AllocateParticularlyLarge(static_cast<size_t>(aligned_size), 1);
            R_UNLESS(static_cast<s64>(pooled_buffer.GetSize()) >= aligned_size, fs::ResultAllocationFailurePooledBufferNotEnoughSize());
            work_buffer = pooled_buffer.GetBuffer();
        }

        /* Ensure cache is coherent. */
        {
            SharedCache cache(this);
            while (cache.AcquireNextOverlappedCache(aligned_offset, aligned_size)) {
                R_TRY(cache.Flush());
                cache.Invalidate();
            }
        }

        /* Read from the base storage. */
        R_TRY(this->base_storage.Read(aligned_offset, work_buffer, static_cast<size_t>(aligned_size)));
        if (work_buffer != static_cast<char *>(buffer)) {
            std::memcpy(buffer, work_buffer + offset - aligned_offset, size);
        }

        bool cached = false;

        /* Handle head cache if needed. */
        if (head_cache_needed) {
            R_TRY(this->PrepareAllocation());

            SharedCache cache(this);
            while (true) {
                R_UNLESS(cache.AcquireFetchableCache(), fs::ResultOutOfResource());

                UniqueCache fetch_cache(this);
                const auto upgrade_result = fetch_cache.Upgrade(cache);
                R_TRY(upgrade_result.first);
                if (upgrade_result.second) {
                    R_TRY(fetch_cache.FetchFromBuffer(aligned_offset, work_buffer, static_cast<size_t>(aligned_size)));
                    break;
                }
            }

            cached = true;
        }

        /* Handle tail cache if needed. */
        if (tail_cache_needed && (!head_cache_needed || aligned_size > static_cast<s64>(this->block_size))) {
            if (!cached) {
                R_TRY(this->PrepareAllocation());
            }

            SharedCache cache(this);
            while (true) {
                R_UNLESS(cache.AcquireFetchableCache(), fs::ResultOutOfResource());

                UniqueCache fetch_cache(this);
                const auto upgrade_result = fetch_cache.Upgrade(cache);
                R_TRY(upgrade_result.first);
                if (upgrade_result.second) {
                    const s64 tail_cache_offset  = util::AlignDown(offset + static_cast<s64>(size), this->block_size);
                    const size_t tail_cache_size = static_cast<size_t>(aligned_size - tail_cache_offset + aligned_offset);
                    R_TRY(fetch_cache.FetchFromBuffer(tail_cache_offset, work_buffer + tail_cache_offset - aligned_offset, tail_cache_size));
                    break;
                }
            }
        }

        if (cached) {
            R_TRY(this->ControlDirtiness());
        }

        return ResultSuccess();
    }

    Result BufferedStorage::WriteCore(s64 offset, const void *buffer, size_t size) {
        AMS_ASSERT(this->caches != nullptr);
        AMS_ASSERT(buffer != nullptr);

        /* Validate the offset. */
        const auto base_storage_size = this->base_storage_size;
        R_UNLESS(offset >= 0,                 fs::ResultInvalidOffset());
        R_UNLESS(offset <= base_storage_size, fs::ResultInvalidOffset());

        /* Setup tracking variables. */
        size_t remaining_size = static_cast<size_t>(std::min<s64>(size, base_storage_size - offset));
        s64 cur_offset        = offset;
        s64 buf_offset        = 0;

        /* Repeatedly read until we're done. */
        while (remaining_size > 0) {
            /* Determine how much to read this iteration. */
            const auto *cur_src = static_cast<const u8 *>(buffer) + buf_offset;
            size_t cur_size = 0;

            if (!util::IsAligned(cur_offset, this->block_size)) {
                const size_t aligned_size = this->block_size - (cur_offset & (this->block_size - 1));
                cur_size = std::min(aligned_size, remaining_size);
            } else if (remaining_size < this->block_size) {
                cur_size = remaining_size;
            } else {
                cur_size = util::AlignDown(remaining_size, this->block_size);
            }

            if (cur_size <= this->block_size) {
                SharedCache cache(this);
                if (!cache.AcquireNextOverlappedCache(cur_offset, cur_size)) {
                    R_TRY(this->PrepareAllocation());
                    while (true) {
                        R_UNLESS(cache.AcquireFetchableCache(), fs::ResultOutOfResource());

                        UniqueCache fetch_cache(this);
                        const auto upgrade_result = fetch_cache.Upgrade(cache);
                        R_TRY(upgrade_result.first);
                        if (upgrade_result.second) {
                            R_TRY(fetch_cache.Fetch(cur_offset));
                            break;
                        }
                    }
                }
                cache.Write(cur_offset, cur_src, cur_size);

                buffers::EnableBlockingBufferManagerAllocation();

                R_TRY(this->ControlDirtiness());
            } else {
                {
                    SharedCache cache(this);
                    while (cache.AcquireNextOverlappedCache(cur_offset, cur_size)) {
                        R_TRY(cache.Flush());
                        cache.Invalidate();
                    }
                }

                R_TRY(this->base_storage.Write(cur_offset, cur_src, cur_size));

                buffers::EnableBlockingBufferManagerAllocation();
            }

            remaining_size -= cur_size;
            cur_offset     += cur_size;
            buf_offset     += cur_size;
        }

        return ResultSuccess();
    }

}
