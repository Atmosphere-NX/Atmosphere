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
#include <stratosphere.hpp>
#include "fssystem_lru_list_cache.hpp"

namespace ams::fssystem {

    class ReadOnlyBlockCacheStorage : public ::ams::fs::IStorage, public ::ams::fs::impl::Newable {
        NON_COPYABLE(ReadOnlyBlockCacheStorage);
        NON_MOVEABLE(ReadOnlyBlockCacheStorage);
        private:
            using BlockCache = LruListCache<s64, char *>;
        private:
            os::SdkMutex m_mutex;
            BlockCache m_block_cache;
            fs::IStorage * const m_base_storage;
            s32 m_block_size;
        public:
            ReadOnlyBlockCacheStorage(IStorage *bs, s32 bsz, char *buf, size_t buf_size, s32 cache_block_count) : m_mutex(), m_block_cache(), m_base_storage(bs), m_block_size(bsz) {
                /* Validate preconditions. */
                AMS_ASSERT(buf_size >= static_cast<size_t>(m_block_size));
                AMS_ASSERT(util::IsPowerOfTwo(m_block_size));
                AMS_ASSERT(cache_block_count > 0);
                AMS_ASSERT(buf_size >= static_cast<size_t>(m_block_size * cache_block_count));
                AMS_UNUSED(buf_size);

                /* Create a node for each cache block. */
                for (auto i = 0; i < cache_block_count; i++) {
                    std::unique_ptr node = std::make_unique<BlockCache::Node>(buf + m_block_size * i);
                    AMS_ASSERT(node != nullptr);

                    if (node != nullptr) {
                        m_block_cache.PushMruNode(std::move(node), -1);
                    }
                }
            }

            ~ReadOnlyBlockCacheStorage() {
                m_block_cache.DeleteAllNodes();
            }

            virtual Result Read(s64 offset, void *buffer, size_t size) override {
                /* Validate preconditions. */
                AMS_ASSERT(util::IsAligned(offset, m_block_size));
                AMS_ASSERT(util::IsAligned(size,   m_block_size));

                if (size == static_cast<size_t>(m_block_size)) {
                    char *cached_buffer = nullptr;

                    /* Try to find a cached copy of the data. */
                    {
                        std::scoped_lock lk(m_mutex);
                        bool found = m_block_cache.FindValueAndUpdateMru(std::addressof(cached_buffer), offset / m_block_size);
                        if (found) {
                            std::memcpy(buffer, cached_buffer, size);
                            return ResultSuccess();
                        }
                    }

                    /* We failed to get a cache hit, so read in the data. */
                    R_TRY(m_base_storage->Read(offset, buffer, size));

                    /* Add the block to the cache. */
                    {
                        std::scoped_lock lk(m_mutex);
                        auto lru = m_block_cache.PopLruNode();
                        std::memcpy(lru->m_value, buffer, m_block_size);
                        m_block_cache.PushMruNode(std::move(lru), offset / m_block_size);
                    }

                    return ResultSuccess();
                } else {
                    return m_base_storage->Read(offset, buffer, size);
                }
            }
            virtual Result OperateRange(void *dst, size_t dst_size, fs::OperationId op_id, s64 offset, s64 size, const void *src, size_t src_size) override {
                /* Validate preconditions. */
                AMS_ASSERT(util::IsAligned(offset, m_block_size));
                AMS_ASSERT(util::IsAligned(size,   m_block_size));

                /* If invalidating cache, invalidate our blocks. */
                if (op_id == fs::OperationId::Invalidate) {
                    R_UNLESS(offset >= 0, fs::ResultInvalidOffset());

                    std::scoped_lock lk(m_mutex);

                    const size_t cache_block_count = m_block_cache.GetSize();
                    BlockCache valid_cache;

                    for (size_t count = 0; count < cache_block_count; ++count) {
                        auto lru = m_block_cache.PopLruNode();
                        if (offset <= lru->m_key && lru->m_key < offset + size) {
                            m_block_cache.PushMruNode(std::move(lru), -1);
                        } else {
                            valid_cache.PushMruNode(std::move(lru), lru->m_key);
                        }
                    }

                    while (!valid_cache.IsEmpty()) {
                        auto lru = valid_cache.PopLruNode();
                        m_block_cache.PushMruNode(std::move(lru), lru->m_key);
                    }
                }

                /* Operate on the base storage. */
                return m_base_storage->OperateRange(dst, dst_size, op_id, offset, size, src, src_size);
            }

            virtual Result GetSize(s64 *out) override {
                return m_base_storage->GetSize(out);
            }

            virtual Result Flush() override {
                return ResultSuccess();
            }

            virtual Result Write(s64 offset, const void *buffer, size_t size) override {
                AMS_UNUSED(offset, buffer, size);
                return fs::ResultUnsupportedOperationInReadOnlyBlockCacheStorageA();
            }

            virtual Result SetSize(s64 size) override {
                AMS_UNUSED(size);
                return fs::ResultUnsupportedOperationInReadOnlyBlockCacheStorageB();
            }
    };

}
