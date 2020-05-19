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
#include <stratosphere/fs/fs_substorage.hpp>

namespace ams::fssystem {

    class BucketTree {
        NON_COPYABLE(BucketTree);
        NON_MOVEABLE(BucketTree);
        public:
            static constexpr u32 Magic   = util::FourCC<'B','K','T','R'>::Code;
            static constexpr u32 Version = 1;

            static constexpr size_t NodeSizeMin = 1_KB;
            static constexpr size_t NodeSizeMax = 512_KB;
        public:
            class Visitor;

            struct Header {
                u32 magic;
                u32 version;
                s32 entry_count;
                s32 reserved;

                void Format(s32 entry_count);
                Result Verify() const;
            };
            static_assert(util::is_pod<Header>::value);
            static_assert(sizeof(Header) == 0x10);

            struct NodeHeader {
                s32 index;
                s32 count;
                s64 offset;

                Result Verify(s32 node_index, size_t node_size, size_t entry_size) const;
            };
            static_assert(util::is_pod<NodeHeader>::value);
            static_assert(sizeof(NodeHeader) == 0x10);

            class ContinuousReadingInfo {
                private:
                    size_t read_size;
                    s32 skip_count;
                    bool done;
                public:
                    constexpr ContinuousReadingInfo() : read_size(), skip_count(), done() { /* ... */ }

                    constexpr void Reset() { this->read_size = 0; this->skip_count = 0; this->done = false; }

                    constexpr void SetSkipCount(s32 count) { AMS_ASSERT(count >= 0); this->skip_count = count; }
                    constexpr s32 GetSkipCount() const { return this->skip_count; }
                    constexpr bool CheckNeedScan() { return (--this->skip_count) <= 0; }

                    constexpr void Done() { this->read_size = 0; this->done = true; }
                    constexpr bool IsDone() const { return this->done; }

                    constexpr void SetReadSize(size_t size) { this->read_size = size; }
                    constexpr size_t GetReadSize() const { return this->read_size; }
                    constexpr bool CanDo() const { return this->read_size > 0; }
            };

            using IAllocator = MemoryResource;
        private:
            class NodeBuffer {
                NON_COPYABLE(NodeBuffer);
                private:
                    IAllocator *allocator;
                    void *header;
                public:
                    NodeBuffer() : allocator(), header() { /* ... */ }

                    ~NodeBuffer() {
                        AMS_ASSERT(this->header == nullptr);
                    }

                    NodeBuffer(NodeBuffer &&rhs) : allocator(rhs.allocator), header(rhs.allocator) {
                        rhs.allocator = nullptr;
                        rhs.header = nullptr;
                    }

                    NodeBuffer &operator=(NodeBuffer &&rhs) {
                        if (this != std::addressof(rhs)) {
                            AMS_ASSERT(this->header == nullptr);

                            this->allocator = rhs.allocator;
                            this->header    = rhs.header;

                            rhs.allocator   = nullptr;
                            rhs.header      = nullptr;
                        }
                        return *this;
                    }

                    bool Allocate(IAllocator *allocator, size_t node_size) {
                        AMS_ASSERT(this->header == nullptr);

                        this->allocator = allocator;
                        this->header    = allocator->Allocate(node_size, sizeof(s64));

                        AMS_ASSERT(util::IsAligned(this->header, sizeof(s64)));

                        return this->header != nullptr;
                    }

                    void Free(size_t node_size) {
                        if (this->header) {
                            this->allocator->Deallocate(this->header, node_size);
                            this->header = nullptr;
                        }
                        this->allocator = nullptr;
                    }

                    void FillZero(size_t node_size) const {
                        if (this->header) {
                            std::memset(this->header, 0, node_size);
                        }
                    }

                    NodeHeader *Get() const {
                        return reinterpret_cast<NodeHeader *>(this->header);
                    }

                    NodeHeader *operator->() const { return this->Get(); }

                    template<typename T>
                    T *Get() const {
                        static_assert(util::is_pod<T>::value);
                        static_assert(sizeof(T) == sizeof(NodeHeader));
                        return reinterpret_cast<T *>(this->header);
                    }

                    IAllocator *GetAllocator() const {
                        return this->allocator;
                    }
            };
        private:
            static constexpr s32 GetEntryCount(size_t node_size, size_t entry_size) {
                return static_cast<s32>((node_size - sizeof(NodeHeader)) / entry_size);
            }

            static constexpr s32 GetOffsetCount(size_t node_size) {
                return static_cast<s32>((node_size - sizeof(NodeHeader)) / sizeof(s64));
            }

            static constexpr s32 GetEntrySetCount(size_t node_size, size_t entry_size, s32 entry_count) {
                const s32 entry_count_per_node = GetEntryCount(node_size, entry_size);
                return util::DivideUp(entry_count, entry_count_per_node);
            }

            static constexpr s32 GetNodeL2Count(size_t node_size, size_t entry_size, s32 entry_count) {
                const s32 offset_count_per_node = GetOffsetCount(node_size);
                const s32 entry_set_count       = GetEntrySetCount(node_size, entry_size, entry_count);

                if (entry_set_count <= offset_count_per_node) {
                    return 0;
                }

                const s32 node_l2_count = util::DivideUp(entry_set_count, offset_count_per_node);
                AMS_ABORT_UNLESS(node_l2_count <= offset_count_per_node);

                return util::DivideUp(entry_set_count - (offset_count_per_node - (node_l2_count - 1)), offset_count_per_node);
            }
        public:
            static constexpr s64 QueryHeaderStorageSize() { return sizeof(Header); }

            static constexpr s64 QueryNodeStorageSize(size_t node_size, size_t entry_size, s32 entry_count) {
                AMS_ASSERT(entry_size >= sizeof(s64));
                AMS_ASSERT(node_size >= entry_size + sizeof(NodeHeader));
                AMS_ASSERT(NodeSizeMin <= node_size && node_size <= NodeSizeMax);
                AMS_ASSERT(util::IsPowerOfTwo(node_size));
                AMS_ASSERT(entry_count >= 0);

                if (entry_count <= 0) {
                    return 0;
                }
                return (1 + GetNodeL2Count(node_size, entry_size, entry_count)) * static_cast<s64>(node_size);
            }

            static constexpr s64 QueryEntryStorageSize(size_t node_size, size_t entry_size, s32 entry_count) {
                AMS_ASSERT(entry_size >= sizeof(s64));
                AMS_ASSERT(node_size >= entry_size + sizeof(NodeHeader));
                AMS_ASSERT(NodeSizeMin <= node_size && node_size <= NodeSizeMax);
                AMS_ASSERT(util::IsPowerOfTwo(node_size));
                AMS_ASSERT(entry_count >= 0);

                if (entry_count <= 0) {
                    return 0;
                }
                return GetEntrySetCount(node_size, entry_size, entry_count) * static_cast<s64>(node_size);
            }
        private:
            mutable fs::SubStorage node_storage;
            mutable fs::SubStorage entry_storage;
            NodeBuffer node_l1;
            size_t node_size;
            size_t entry_size;
            s32 entry_count;
            s32 offset_count;
            s32 entry_set_count;
            s64 start_offset;
            s64 end_offset;
        public:
            BucketTree() : node_storage(), entry_storage(), node_l1(), node_size(), entry_size(), entry_count(), offset_count(), entry_set_count(), start_offset(), end_offset() { /* ... */ }
            ~BucketTree() { this->Finalize(); }

            Result Initialize(IAllocator *allocator, fs::SubStorage node_storage, fs::SubStorage entry_storage, size_t node_size, size_t entry_size, s32 entry_count);
            void Initialize(size_t node_size, s64 end_offset);
            void Finalize();

            bool IsInitialized() const { return this->node_size > 0; }
            bool IsEmpty() const { return this->entry_size == 0; }

            Result Find(Visitor *visitor, s64 virtual_address) const;
            Result InvalidateCache();

            s32 GetEntryCount() const { return this->entry_count; }
            IAllocator *GetAllocator() const { return this->node_l1.GetAllocator(); }

            s64 GetStart() const { return this->start_offset; }
            s64 GetEnd() const { return this->end_offset; }
            s64 GetSize() const { return this->end_offset - this->start_offset; }

            bool Includes(s64 offset) const {
                return this->start_offset <= offset && offset < this->end_offset;
            }

            bool Includes(s64 offset, s64 size) const {
                return size > 0 && this->start_offset <= offset && size <= this->end_offset - offset;
            }
        private:
            template<typename EntryType>
            struct ContinuousReadingParam {
                s64 offset;
                size_t size;
                NodeHeader entry_set;
                s32 entry_index;
                EntryType entry;
            };
        private:
            template<typename EntryType>
            Result ScanContinuousReading(ContinuousReadingInfo *out_info, const ContinuousReadingParam<EntryType> &param) const;

            bool IsExistL2() const { return this->offset_count < this->entry_set_count; }
            bool IsExistOffsetL2OnL1() const { return this->IsExistL2() && this->node_l1->count < this->offset_count; }

            s64 GetEntrySetIndex(s32 node_index, s32 offset_index) const {
                return (this->offset_count - this->node_l1->count) + (this->offset_count * node_index) + offset_index;
            }
    };

    class BucketTree::Visitor {
        NON_COPYABLE(Visitor);
        NON_MOVEABLE(Visitor);
        private:
            friend class BucketTree;

            union EntrySetHeader {
                NodeHeader header;
                struct Info {
                    s32 index;
                    s32 count;
                    s64 end;
                    s64 start;
                } info;
                static_assert(util::is_pod<Info>::value);
            };
            static_assert(util::is_pod<EntrySetHeader>::value);
        private:
            const BucketTree *tree;
            void *entry;
            s32 entry_index;
            s32 entry_set_count;
            EntrySetHeader entry_set;
        public:
            constexpr Visitor() : tree(), entry(), entry_index(-1), entry_set_count(), entry_set{} { /* ... */ }
            ~Visitor() {
                if (this->entry != nullptr) {
                    this->tree->GetAllocator()->Deallocate(this->entry, this->tree->entry_size);
                    this->tree  = nullptr;
                    this->entry = nullptr;
                }
            }

            bool IsValid() const { return this->entry_index >= 0; }
            bool CanMoveNext() const { return this->IsValid() && (this->entry_index + 1 < this->entry_set.info.count || this->entry_set.info.index + 1 < this->entry_set_count); }
            bool CanMovePrevious() const { return this->IsValid() && (this->entry_index > 0 || this->entry_set.info.index > 0); }

            Result MoveNext();
            Result MovePrevious();

            template<typename EntryType>
            Result ScanContinuousReading(ContinuousReadingInfo *out_info, s64 offset, size_t size) const;

            const void *Get() const { AMS_ASSERT(this->IsValid()); return this->entry; }

            template<typename T>
            const T *Get() const { AMS_ASSERT(this->IsValid()); return reinterpret_cast<const T *>(this->entry); }

            const BucketTree *GetTree() const { return this->tree; }
        private:
            Result Initialize(const BucketTree *tree);

            Result Find(s64 virtual_address);

            Result FindEntrySet(s32 *out_index, s64 virtual_address, s32 node_index);
            Result FindEntrySetWithBuffer(s32 *out_index, s64 virtual_address, s32 node_index, char *buffer);
            Result FindEntrySetWithoutBuffer(s32 *out_index, s64 virtual_address, s32 node_index);

            Result FindEntry(s64 virtual_address, s32 entry_set_index);
            Result FindEntryWithBuffer(s64 virtual_address, s32 entry_set_index, char *buffer);
            Result FindEntryWithoutBuffer(s64 virtual_address, s32 entry_set_index);
    };

}
