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
#include <stratosphere/fssystem/fssystem_asynchronous_access.hpp>
#include <stratosphere/fssystem/fssystem_bucket_tree.hpp>
#include <stratosphere/fssystem/fssystem_compression_common.hpp>

namespace ams::fssystem {

    class CompressedStorage : public ::ams::fs::IStorage, public ::ams::fssystem::IAsynchronousAccessSplitter, public ::ams::fs::impl::Newable {
        NON_COPYABLE(CompressedStorage);
        NON_MOVEABLE(CompressedStorage);
        public:
            static constexpr size_t NodeSize  = 16_KB;

            using IAllocator = BucketTree::IAllocator;

            struct Entry {
                s64 virt_offset;
                s64 phys_offset;
                CompressionType compression_type;
                s32 phys_size;

                s64 GetPhysicalSize() const {
                    return this->phys_size;
                }
            };
            static_assert(util::is_pod<Entry>::value);
            static_assert(sizeof(Entry) == 0x18);
        public:
            static constexpr s64 QueryNodeStorageSize(s32 entry_count) {
                return BucketTree::QueryNodeStorageSize(NodeSize, sizeof(Entry), entry_count);
            }

            static constexpr s64 QueryEntryStorageSize(s32 entry_count) {
                return BucketTree::QueryEntryStorageSize(NodeSize, sizeof(Entry), entry_count);
            }
        private:
            /* TODO: CompressedStorageCore m_core; */
            /* TODO: CacheManager m_cache_manager; */
        public:
            CompressedStorage() { /* ... */ }
            virtual ~CompressedStorage() { this->Finalize(); }

            Result Initialize(MemoryResource *bktr_allocator, IBufferManager *cache_allocator, fs::SubStorage data_storage, fs::SubStorage node_storage, fs::SubStorage entry_storage, s32 bktr_entry_count, size_t block_size_max, size_t continuous_reading_size_max, GetDecompressorFunction get_decompressor, size_t cache_size_0, size_t cache_size_1, s32 max_cache_entries);

            void Finalize() {
                AMS_ABORT("TODO");
                /* m_cache_manager.Finalize(); */
                /* m_core.Finalize(); */
            }
        public:
            virtual Result QueryAppropriateOffset(s64 *out, s64 offset, s64 access_size, s64 alignment_size) override {
                AMS_ABORT("TODO");
                AMS_UNUSED(out, offset, access_size, alignment_size);
                /* return m_core.QueryAppropriateOffsetForAsynchronousAccess(out, offset, access_size, alignment_size); */
            }
        public:
            virtual Result Read(s64 offset, void *buffer, size_t size) override;
            virtual Result OperateRange(void *dst, size_t dst_size, fs::OperationId op_id, s64 offset, s64 size, const void *src, size_t src_size) override;

            virtual Result GetSize(s64 *out) override {
                AMS_ABORT("TODO");
                AMS_UNUSED(out);
                /* return m_core.GetSize(out); */
            }

            virtual Result Flush() override {
                return ResultSuccess();
            }

            virtual Result Write(s64 offset, const void *buffer, size_t size) override {
                AMS_UNUSED(offset, buffer, size);
                return fs::ResultUnsupportedOperationInCompressedStorageA();
            }

            virtual Result SetSize(s64 size) override {
                AMS_UNUSED(size);
                /* NOTE: Is Nintendo returning the wrong result here? */
                return fs::ResultUnsupportedOperationInIndirectStorageB();
            }
    };

}
