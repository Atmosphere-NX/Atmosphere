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
#include "fssystem_lru_list_cache.hpp"

namespace ams::fssystem {

    class ReadOnlyBlockCacheStorage : public ::ams::fs::IStorage, public ::ams::fs::impl::Newable {
        NON_COPYABLE(ReadOnlyBlockCacheStorage);
        NON_MOVEABLE(ReadOnlyBlockCacheStorage);
        private:
            using BlockCache = LruListCache<s64, char *>;
        private:
            os::Mutex mutex;
            BlockCache block_cache;
            fs::IStorage * const base_storage;
            s32 block_size;
        public:
            ReadOnlyBlockCacheStorage(IStorage *bs, s32 bsz, char *buf, size_t buf_size, s32 cache_block_count) : mutex(false), base_storage(bs), block_size(bsz) {
                /* Validate preconditions. */
                AMS_ASSERT(buf_size >= static_cast<size_t>(this->block_size));
                AMS_ASSERT(util::IsPowerOfTwo(this->block_size));
                AMS_ASSERT(cache_block_count > 0);
                AMS_ASSERT(buf_size >= static_cast<size_t>(this->block_size * cache_block_count));

                /* Create a node for each cache block. */
                for (auto i = 0; i < cache_block_count; i++) {
                    std::unique_ptr node = std::make_unique<BlockCache::Node>(buf + this->block_size * i);
                    AMS_ASSERT(node != nullptr);

                    if (node != nullptr) {
                        this->block_cache.PushMruNode(std::move(node), -1);
                    }
                }
            }

            ~ReadOnlyBlockCacheStorage() {
                this->block_cache.DeleteAllNodes();
            }

            virtual Result Read(s64 offset, void *buffer, size_t size) override {
                /* Validate preconditions. */
                AMS_ASSERT(util::IsAligned(offset, this->block_size));
                AMS_ASSERT(util::IsAligned(size,   this->block_size));

                if (size == static_cast<size_t>(this->block_size)) {
                    char *cached_buffer = nullptr;

                    /* Try to find a cached copy of the data. */
                    {
                        std::scoped_lock lk(this->mutex);
                        bool found = this->block_cache.FindValueAndUpdateMru(std::addressof(cached_buffer), offset / this->block_size);
                        if (found) {
                            std::memcpy(buffer, cached_buffer, size);
                            return ResultSuccess();
                        }
                    }

                    /* We failed to get a cache hit, so read in the data. */
                    R_TRY(this->base_storage->Read(offset, buffer, size));

                    /* Add the block to the cache. */
                    {
                        std::scoped_lock lk(this->mutex);
                        auto lru = this->block_cache.PopLruNode();
                        std::memcpy(lru->value, buffer, this->block_size);
                        this->block_cache.PushMruNode(std::move(lru), offset / this->block_size);
                    }

                    return ResultSuccess();
                } else {
                    return this->base_storage->Read(offset, buffer, size);
                }
            }
            virtual Result OperateRange(void *dst, size_t dst_size, fs::OperationId op_id, s64 offset, s64 size, const void *src, size_t src_size) override {
                /* Validate preconditions. */
                AMS_ASSERT(util::IsAligned(offset, this->block_size));
                AMS_ASSERT(util::IsAligned(size,   this->block_size));

                /* If invalidating cache, invalidate our blocks. */
                if (op_id == fs::OperationId::InvalidateCache) {
                    R_UNLESS(offset >= 0, fs::ResultInvalidOffset());

                    std::scoped_lock lk(this->mutex);

                    const size_t cache_block_count = this->block_cache.GetSize();
                    BlockCache valid_cache;

                    for (size_t count = 0; count < cache_block_count; ++count) {
                        auto lru = this->block_cache.PopLruNode();
                        if (offset <= lru->key && lru->key < offset + size) {
                            this->block_cache.PushMruNode(std::move(lru), -1);
                        } else {
                            valid_cache.PushMruNode(std::move(lru), lru->key);
                        }
                    }

                    while (!valid_cache.IsEmpty()) {
                        auto lru = valid_cache.PopLruNode();
                        this->block_cache.PushMruNode(std::move(lru), lru->key);
                    }
                }

                /* Operate on the base storage. */
                return this->base_storage->OperateRange(dst, dst_size, op_id, offset, size, src, src_size);
            }

            virtual Result GetSize(s64 *out) override {
                return this->base_storage->GetSize(out);
            }

            virtual Result Flush() override {
                return ResultSuccess();
            }

            virtual Result Write(s64 offset, const void *buffer, size_t size) override {
                return fs::ResultUnsupportedOperationInReadOnlyBlockCacheStorageA();
            }

            virtual Result SetSize(s64 size) override {
                return fs::ResultUnsupportedOperationInReadOnlyBlockCacheStorageB();
            }
    };

}
