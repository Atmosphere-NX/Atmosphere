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
#include <vapours.hpp>
#include <stratosphere/fssystem/fssystem_bucket_tree.hpp>

namespace ams::fssystem {

    class IndirectStorage : public ::ams::fs::IStorage, public ::ams::fs::impl::Newable {
        NON_COPYABLE(IndirectStorage);
        NON_MOVEABLE(IndirectStorage);
        public:
            static constexpr s32 StorageCount = 2;
            static constexpr size_t NodeSize = 16_KB;

            using IAllocator = MemoryResource;

            struct Entry {
                u8 virt_offset[sizeof(s64)];
                u8 phys_offset[sizeof(s64)];
                s32 storage_index;

                void SetVirtualOffset(const s64 &ofs) {
                    std::memcpy(this->virt_offset, std::addressof(ofs), sizeof(s64));
                }

                s64 GetVirtualOffset() const {
                    s64 offset;
                    std::memcpy(std::addressof(offset), this->virt_offset, sizeof(s64));
                    return offset;
                }

                void SetPhysicalOffset(const s64 &ofs) {
                    std::memcpy(this->phys_offset, std::addressof(ofs), sizeof(s64));
                }

                s64 GetPhysicalOffset() const {
                    s64 offset;
                    std::memcpy(std::addressof(offset), this->phys_offset, sizeof(s64));
                    return offset;
                }
            };
            static_assert(util::is_pod<Entry>::value);
            static_assert(sizeof(Entry) == 0x14);

            struct EntryData {
                s64 virt_offset;
                s64 phys_offset;
                s32 storage_index;

                void Set(const Entry &entry) {
                    this->virt_offset   = entry.GetVirtualOffset();
                    this->phys_offset   = entry.GetPhysicalOffset();
                    this->storage_index = entry.storage_index;
                }
            };
            static_assert(util::is_pod<EntryData>::value);
        private:
            struct ContinuousReadingEntry {
                static constexpr size_t FragmentSizeMax = 4_KB;

                IndirectStorage::Entry entry;

                s64 GetVirtualOffset() const {
                    return this->entry.GetVirtualOffset();
                }

                s64 GetPhysicalOffset() const {
                    return this->entry.GetPhysicalOffset();
                }

                bool IsFragment() const {
                    return this->entry.storage_index != 0;
                }
            };
            static_assert(util::is_pod<ContinuousReadingEntry>::value);
        public:
            static constexpr s64 QueryHeaderStorageSize() {
                return BucketTree::QueryHeaderStorageSize();
            }

            static constexpr s64 QueryNodeStorageSize(s32 entry_count) {
                return BucketTree::QueryNodeStorageSize(NodeSize, sizeof(Entry), entry_count);
            }

            static constexpr s64 QueryEntryStorageSize(s32 entry_count) {
                return BucketTree::QueryEntryStorageSize(NodeSize, sizeof(Entry), entry_count);
            }
        private:
            BucketTree table;
            fs::SubStorage data_storage[StorageCount];
        public:
            IndirectStorage() : table(), data_storage() { /* ... */ }
            virtual ~IndirectStorage() { this->Finalize(); }

            Result Initialize(IAllocator *allocator, fs::SubStorage table_storage);
            void Finalize();

            bool IsInitialized() const { return this->table.IsInitialized(); }

            Result Initialize(IAllocator *allocator, fs::SubStorage node_storage, fs::SubStorage entry_storage, s32 entry_count) {
                return this->table.Initialize(allocator, node_storage, entry_storage, NodeSize, sizeof(Entry), entry_count);
            }

            void SetStorage(s32 idx, fs::SubStorage storage) {
                AMS_ASSERT(0 <= idx && idx < StorageCount);
                this->data_storage[idx] = storage;
            }

            template<typename T>
            void SetStorage(s32 idx, T storage, s64 offset, s64 size) {
                AMS_ASSERT(0 <= idx && idx < StorageCount);
                this->data_storage[idx] = fs::SubStorage(storage, offset, size);
            }

            Result GetEntryList(Entry *out_entries, s32 *out_entry_count, s32 entry_count, s64 offset, s64 size);

            virtual Result Read(s64 offset, void *buffer, size_t size) override;
            virtual Result OperateRange(void *dst, size_t dst_size, fs::OperationId op_id, s64 offset, s64 size, const void *src, size_t src_size) override;

            virtual Result GetSize(s64 *out) override {
                AMS_ASSERT(out != nullptr);
                *out = this->table.GetEnd();
                return ResultSuccess();
            }

            virtual Result Flush() override {
                return ResultSuccess();
            }

            virtual Result Write(s64 offset, const void *buffer, size_t size) override {
                return fs::ResultUnsupportedOperationInIndirectStorageA();
            }

            virtual Result SetSize(s64 size) override {
                return fs::ResultUnsupportedOperationInIndirectStorageB();
            }
        protected:
            BucketTree &GetEntryTable() { return this->table; }

            fs::SubStorage &GetDataStorage(s32 index) {
                AMS_ASSERT(0 <= index && index < StorageCount);
                return this->data_storage[index];
            }

            template<bool ContinuousCheck, typename F>
            Result OperatePerEntry(s64 offset, s64 size, F func);
    };

}
