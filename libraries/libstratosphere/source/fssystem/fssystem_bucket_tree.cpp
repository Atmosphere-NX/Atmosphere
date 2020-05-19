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

namespace ams::fssystem {

    namespace {

        using Node = impl::BucketTreeNode<const s64 *>;
        static_assert(sizeof(Node) == sizeof(BucketTree::NodeHeader));
        static_assert(util::is_pod<Node>::value);

        constexpr inline s32 NodeHeaderSize = sizeof(BucketTree::NodeHeader);

        class StorageNode {
            private:
                class Offset {
                    public:
                        using difference_type = s64;
                    private:
                        s64 offset;
                        s32 stride;
                    public:
                        constexpr Offset(s64 offset, s32 stride) : offset(offset), stride(stride) { /* ... */ }

                        constexpr Offset &operator++() { this->offset += this->stride; return *this; }
                        constexpr Offset operator++(int) { Offset ret(*this); this->offset += this->stride; return ret; }

                        constexpr Offset &operator--() { this->offset -= this->stride; return *this; }
                        constexpr Offset operator--(int) { Offset ret(*this); this->offset -= this->stride; return ret; }

                        constexpr difference_type operator-(const Offset &rhs) const { return (this->offset - rhs.offset) / this->stride; }

                        constexpr Offset operator+(difference_type ofs) const { return Offset(this->offset + ofs * this->stride, this->stride); }
                        constexpr Offset operator-(difference_type ofs) const { return Offset(this->offset - ofs * this->stride, this->stride); }

                        constexpr Offset &operator+=(difference_type ofs) { this->offset += ofs * this->stride; return *this; }
                        constexpr Offset &operator-=(difference_type ofs) { this->offset -= ofs * this->stride; return *this; }

                        constexpr bool operator==(const Offset &rhs) const { return this->offset == rhs.offset; }
                        constexpr bool operator!=(const Offset &rhs) const { return this->offset != rhs.offset; }

                        constexpr s64 Get() const { return this->offset; }
                };
            private:
                const Offset start;
                const s32 count;
                s32 index;
            public:
                StorageNode(size_t size, s32 count) : start(NodeHeaderSize, static_cast<s32>(size)), count(count), index(-1) { /* ... */ }
                StorageNode(s64 ofs, size_t size, s32 count) : start(NodeHeaderSize + ofs, static_cast<s32>(size)), count(count), index(-1) { /* ... */ }

                s32 GetIndex() const { return this->index; }

                void Find(const char *buffer, s64 virtual_address) {
                    s32 end = this->count;
                    auto pos  = this->start;

                    while (end > 0) {
                        auto half = end / 2;
                        auto mid  = pos + half;

                        s64 offset = 0;
                        std::memcpy(std::addressof(offset), buffer + mid.Get(), sizeof(s64));

                        if (offset <= virtual_address) {
                            pos = mid + 1;
                            end -= half + 1;
                        } else {
                            end = half;
                        }
                    }

                    this->index = static_cast<s32>(pos - this->start) - 1;
                }

                Result Find(fs::SubStorage &storage, s64 virtual_address) {
                    s32 end = this->count;
                    auto pos  = this->start;

                    while (end > 0) {
                        auto half = end / 2;
                        auto mid  = pos + half;

                        s64 offset = 0;
                        R_TRY(storage.Read(mid.Get(), std::addressof(offset), sizeof(s64)));

                        if (offset <= virtual_address) {
                            pos = mid + 1;
                            end -= half + 1;
                        } else {
                            end = half;
                        }
                    }

                    this->index = static_cast<s32>(pos - this->start) - 1;
                    return ResultSuccess();
                }
        };

    }

    void BucketTree::Header::Format(s32 entry_count) {
        AMS_ASSERT(entry_count >= 0);

        this->magic       = Magic;
        this->version     = Version;
        this->entry_count = entry_count;
        this->reserved    = 0;
    }

    Result BucketTree::Header::Verify() const {
        R_UNLESS(this->magic == Magic,     fs::ResultInvalidBucketTreeSignature());
        R_UNLESS(this->entry_count >= 0,   fs::ResultInvalidBucketTreeEntryCount());
        R_UNLESS(this->version <= Version, fs::ResultUnsupportedVersion());
        return ResultSuccess();
    }

    Result BucketTree::NodeHeader::Verify(s32 node_index, size_t node_size, size_t entry_size) const {
        R_UNLESS(this->index == node_index,                                   fs::ResultInvalidBucketTreeNodeIndex());
        R_UNLESS(entry_size != 0 && node_size >= entry_size + NodeHeaderSize, fs::ResultInvalidSize());

        const size_t max_entry_count = (node_size - NodeHeaderSize) / entry_size;
        R_UNLESS(this->count > 0 && static_cast<size_t>(this->count) <= max_entry_count, fs::ResultInvalidBucketTreeNodeEntryCount());
        R_UNLESS(this->offset >= 0,                                                      fs::ResultInvalidBucketTreeNodeOffset());

        return ResultSuccess();
    }

    Result BucketTree::Initialize(IAllocator *allocator, fs::SubStorage node_storage, fs::SubStorage entry_storage, size_t node_size, size_t entry_size, s32 entry_count) {
        /* Validate preconditions. */
        AMS_ASSERT(allocator != nullptr);
        AMS_ASSERT(entry_size >= sizeof(s64));
        AMS_ASSERT(node_size >= entry_size + sizeof(NodeHeader));
        AMS_ASSERT(NodeSizeMin <= node_size && node_size <= NodeSizeMax);
        AMS_ASSERT(util::IsPowerOfTwo(node_size));
        AMS_ASSERT(!this->IsInitialized());

        /* Ensure valid entry count. */
        R_UNLESS(entry_count > 0, fs::ResultInvalidArgument());

        /* Allocate node. */
        R_UNLESS(this->node_l1.Allocate(allocator, node_size), fs::ResultBufferAllocationFailed());
        auto node_guard = SCOPE_GUARD { this->node_l1.Free(node_size); };

        /* Read node. */
        R_TRY(node_storage.Read(0, this->node_l1.Get(), node_size));

        /* Verify node. */
        R_TRY(this->node_l1->Verify(0, node_size, sizeof(s64)));

        /* Validate offsets. */
        const auto offset_count = GetOffsetCount(node_size);
        const auto entry_set_count = GetEntrySetCount(node_size, entry_size, entry_count);
        const auto * const node = this->node_l1.Get<Node>();

        s64 start_offset;
        if (offset_count < entry_set_count && node->GetCount() < offset_count) {
            start_offset = *node->GetEnd();
        } else {
            start_offset = *node->GetBegin();
        }
        const auto end_offset = node->GetEndOffset();

        R_UNLESS(0 <= start_offset && start_offset <= node->GetBeginOffset(), fs::ResultInvalidBucketTreeEntryOffset());
        R_UNLESS(start_offset < end_offset,                                   fs::ResultInvalidBucketTreeEntryOffset());

        /* Set member variables. */
        this->node_storage    = node_storage;
        this->entry_storage   = entry_storage;
        this->node_size       = node_size;
        this->entry_size      = entry_size;
        this->entry_count     = entry_count;
        this->offset_count    = offset_count;
        this->entry_set_count = entry_set_count;
        this->start_offset    = start_offset;
        this->end_offset      = end_offset;

        /* Cancel guard. */
        node_guard.Cancel();
        return ResultSuccess();
    }

    void BucketTree::Initialize(size_t node_size, s64 end_offset) {
        AMS_ASSERT(NodeSizeMin <= node_size && node_size <= NodeSizeMax);
        AMS_ASSERT(util::IsPowerOfTwo(node_size));
        AMS_ASSERT(end_offset > 0);
        AMS_ASSERT(!this->IsInitialized());

        this->node_size  = node_size;
        this->end_offset = end_offset;
    }

    void BucketTree::Finalize() {
        if (this->IsInitialized()) {
            this->node_storage    = fs::SubStorage();
            this->entry_storage   = fs::SubStorage();
            this->node_l1.Free(this->node_size);
            this->node_size       = 0;
            this->entry_size      = 0;
            this->entry_count     = 0;
            this->offset_count    = 0;
            this->entry_set_count = 0;
            this->start_offset    = 0;
            this->end_offset      = 0;
        }
    }

    Result BucketTree::Find(Visitor *visitor, s64 virtual_address) const {
        AMS_ASSERT(visitor != nullptr);
        AMS_ASSERT(this->IsInitialized());

        R_UNLESS(virtual_address >= 0, fs::ResultInvalidOffset());
        R_UNLESS(!this->IsEmpty(),     fs::ResultOutOfRange());

        R_TRY(visitor->Initialize(this));

        return visitor->Find(virtual_address);
    }

    Result BucketTree::InvalidateCache() {
        /* Invalidate the node storage cache. */
        {
            s64 storage_size;
            R_TRY(this->node_storage.GetSize(std::addressof(storage_size)));
            R_TRY(this->node_storage.OperateRange(fs::OperationId::InvalidateCache, 0, storage_size));
        }

        /* Refresh start/end offsets. */
        {
            /* Read node. */
            R_TRY(node_storage.Read(0, this->node_l1.Get(), this->node_size));

            /* Verify node. */
            R_TRY(this->node_l1->Verify(0, this->node_size, sizeof(s64)));

            /* Validate offsets. */
            const auto * const node = this->node_l1.Get<Node>();

            s64 start_offset;
            if (offset_count < this->entry_set_count && node->GetCount() < this->offset_count) {
                start_offset = *node->GetEnd();
            } else {
                start_offset = *node->GetBegin();
            }
            const auto end_offset = node->GetEndOffset();

            R_UNLESS(0 <= start_offset && start_offset <= node->GetBeginOffset(), fs::ResultInvalidBucketTreeEntryOffset());
            R_UNLESS(start_offset < end_offset,                                   fs::ResultInvalidBucketTreeEntryOffset());

            /* Set refreshed offsets. */
            this->start_offset = start_offset;
            this->end_offset   = end_offset;
        }

        /* Invalidate the entry storage cache. */
        {
            s64 storage_size;
            R_TRY(this->entry_storage.GetSize(std::addressof(storage_size)));
            R_TRY(this->entry_storage.OperateRange(fs::OperationId::InvalidateCache, 0, storage_size));
        }

        return ResultSuccess();
    }

    Result BucketTree::Visitor::Initialize(const BucketTree *tree) {
        AMS_ASSERT(tree != nullptr);
        AMS_ASSERT(this->tree == nullptr || this->tree == tree);

        if (this->entry == nullptr) {
            this->entry = tree->GetAllocator()->Allocate(tree->entry_size);
            R_UNLESS(this->entry != nullptr, fs::ResultBufferAllocationFailed());

            this->tree = tree;
        }

        return ResultSuccess();
    }

    Result BucketTree::Visitor::MoveNext() {
        R_UNLESS(this->IsValid(), fs::ResultOutOfRange());

        /* Invalidate our index, and read the header for the next index. */
        auto entry_index = this->entry_index + 1;
        if (entry_index == this->entry_set.info.count) {
            const auto entry_set_index = this->entry_set.info.index + 1;
            R_UNLESS(entry_set_index < this->entry_set_count, fs::ResultOutOfRange());

            this->entry_index = -1;

            const auto end = this->entry_set.info.end;

            const auto entry_set_size   = this->tree->node_size;
            const auto entry_set_offset = entry_set_index * static_cast<s64>(entry_set_size);

            R_TRY(this->tree->entry_storage.Read(entry_set_offset, std::addressof(this->entry_set), sizeof(EntrySetHeader)));
            R_TRY(this->entry_set.header.Verify(entry_set_index, entry_set_size, this->tree->entry_size));

            R_UNLESS(this->entry_set.info.start == end && this->entry_set.info.start < this->entry_set.info.end, fs::ResultInvalidBucketTreeEntrySetOffset());

            entry_index = 0;
        } else {
            this->entry_index = 1;
        }

        /* Read the new entry. */
        const auto entry_size   = this->tree->entry_size;
        const auto entry_offset = impl::GetBucketTreeEntryOffset(this->entry_set.info.index, this->tree->node_size, entry_size, entry_index);
        R_TRY(this->tree->entry_storage.Read(entry_offset, std::addressof(this->entry), entry_size));

        /* Note that we changed index. */
        this->entry_index = entry_index;
        return ResultSuccess();
    }

    Result BucketTree::Visitor::MovePrevious() {
        R_UNLESS(this->IsValid(), fs::ResultOutOfRange());

        /* Invalidate our index, and read the heasder for the previous index. */
        auto entry_index = this->entry_index;
        if (entry_index == 0) {
            R_UNLESS(this->entry_set.info.index > 0, fs::ResultOutOfRange());

            this->entry_index = -1;

            const auto start = this->entry_set.info.start;

            const auto entry_set_size   = this->tree->node_size;
            const auto entry_set_index  = this->entry_set.info.index - 1;
            const auto entry_set_offset = entry_set_index * static_cast<s64>(entry_set_size);

            R_TRY(this->tree->entry_storage.Read(entry_set_offset, std::addressof(this->entry_set), sizeof(EntrySetHeader)));
            R_TRY(this->entry_set.header.Verify(entry_set_index, entry_set_size, this->tree->entry_size));

            R_UNLESS(this->entry_set.info.end == start && this->entry_set.info.start < this->entry_set.info.end, fs::ResultInvalidBucketTreeEntrySetOffset());

            entry_index = this->entry_set.info.count;
        } else {
            this->entry_index = -1;
        }

        --entry_index;

        /* Read the new entry. */
        const auto entry_size   = this->tree->entry_size;
        const auto entry_offset = impl::GetBucketTreeEntryOffset(this->entry_set.info.index, this->tree->node_size, entry_size, entry_index);
        R_TRY(this->tree->entry_storage.Read(entry_offset, std::addressof(this->entry), entry_size));

        /* Note that we changed index. */
        this->entry_index = entry_index;
        return ResultSuccess();
    }

    Result BucketTree::Visitor::Find(s64 virtual_address) {
        AMS_ASSERT(this->tree != nullptr);

        /* Get the node. */
        const auto * const node = this->tree->node_l1.Get<Node>();
        R_UNLESS(virtual_address < node->GetEndOffset(), fs::ResultOutOfRange());

        /* Get the entry set index. */
        s32 entry_set_index = -1;
        if (this->tree->IsExistOffsetL2OnL1() && virtual_address < node->GetBeginOffset()) {
            const auto start = node->GetEnd();
            const auto end   = node->GetBegin() + tree->offset_count;

            auto pos = std::upper_bound(start, end, virtual_address);
            R_UNLESS(start < pos, fs::ResultOutOfRange());
            --pos;

            entry_set_index = static_cast<s32>(pos - start);
        } else {
            const auto start = node->GetBegin();
            const auto end   = node->GetEnd();

            auto pos = std::upper_bound(start, end, virtual_address);
            R_UNLESS(start < pos, fs::ResultOutOfRange());
            --pos;

            if (this->tree->IsExistL2()) {
                const auto node_index = static_cast<s32>(pos - start);
                R_UNLESS(0 <= node_index && node_index < this->tree->offset_count, fs::ResultInvalidBucketTreeNodeOffset());

                R_TRY(this->FindEntrySet(std::addressof(entry_set_index), virtual_address, node_index));
            } else {
                entry_set_index = static_cast<s32>(pos - start);
            }
        }

        /* Validate the entry set index. */
        R_UNLESS(0 <= entry_set_index && entry_set_index < this->tree->entry_set_count, fs::ResultInvalidBucketTreeNodeOffset());

        /* Find the entry. */
        R_TRY(this->FindEntry(virtual_address, entry_set_index));

        /* Set count. */
        this->entry_set_count = this->tree->entry_set_count;
        return ResultSuccess();
    }

    Result BucketTree::Visitor::FindEntrySet(s32 *out_index, s64 virtual_address, s32 node_index) {
        const auto node_size = this->tree->node_size;

        PooledBuffer pool(node_size, 1);
        if (node_size <= pool.GetSize()) {
            return this->FindEntrySetWithBuffer(out_index, virtual_address, node_index, pool.GetBuffer());
        } else {
            pool.Deallocate();
            return this->FindEntrySetWithoutBuffer(out_index, virtual_address, node_index);
        }
    }

    Result BucketTree::Visitor::FindEntrySetWithBuffer(s32 *out_index, s64 virtual_address, s32 node_index, char *buffer) {
        /* Calculate node extents. */
        const auto node_size = this->tree->node_size;
        const auto node_offset = (node_index + 1) * static_cast<s64>(node_size);
        fs::SubStorage &storage = tree->node_storage;

        /* Read the node. */
        R_TRY(storage.Read(node_offset, buffer, node_size));

        /* Validate the header. */
        NodeHeader header;
        std::memcpy(std::addressof(header), buffer, NodeHeaderSize);
        R_TRY(header.Verify(node_index, node_size, sizeof(s64)));

        /* Create the node, and find. */
        StorageNode node(sizeof(s64), header.count);
        node.Find(buffer, virtual_address);
        R_UNLESS(node.GetIndex() >= 0, fs::ResultInvalidBucketTreeVirtualOffset());

        /* Return the index. */
        *out_index = this->tree->GetEntrySetIndex(header.index, node.GetIndex());
        return ResultSuccess();
    }

    Result BucketTree::Visitor::FindEntrySetWithoutBuffer(s32 *out_index, s64 virtual_address, s32 node_index) {
        /* Calculate node extents. */
        const auto node_size = this->tree->node_size;
        const auto node_offset = (node_index + 1) * static_cast<s64>(node_size);
        fs::SubStorage &storage = tree->node_storage;

        /* Read and validate the header. */
        NodeHeader header;
        R_TRY(storage.Read(node_offset, std::addressof(header), NodeHeaderSize));
        R_TRY(header.Verify(node_index, node_size, sizeof(s64)));

        /* Create the node, and find. */
        StorageNode node(node_offset, sizeof(s64), header.count);
        R_TRY(node.Find(storage, virtual_address));
        R_UNLESS(node.GetIndex() >= 0, fs::ResultOutOfRange());

        /* Return the index. */
        *out_index = this->tree->GetEntrySetIndex(header.index, node.GetIndex());
        return ResultSuccess();
    }

    Result BucketTree::Visitor::FindEntry(s64 virtual_address, s32 entry_set_index) {
        const auto entry_set_size = this->tree->node_size;

        PooledBuffer pool(entry_set_size, 1);
        if (entry_set_size <= pool.GetSize()) {
            return this->FindEntryWithBuffer(virtual_address, entry_set_index, pool.GetBuffer());
        } else {
            pool.Deallocate();
            return this->FindEntryWithoutBuffer(virtual_address, entry_set_index);
        }
    }

    Result BucketTree::Visitor::FindEntryWithBuffer(s64 virtual_address, s32 entry_set_index, char *buffer) {
        /* Calculate entry set extents. */
        const auto entry_size       = this->tree->entry_size;
        const auto entry_set_size   = this->tree->node_size;
        const auto entry_set_offset = entry_set_index * static_cast<s64>(entry_set_size);
        fs::SubStorage &storage = tree->entry_storage;

        /* Read the entry set. */
        R_TRY(storage.Read(entry_set_offset, buffer, entry_set_size));

        /* Validate the entry_set. */
        EntrySetHeader entry_set;
        std::memcpy(std::addressof(entry_set), buffer, sizeof(EntrySetHeader));
        R_TRY(entry_set.header.Verify(entry_set_index, entry_set_size, entry_size));

        /* Create the node, and find. */
        StorageNode node(entry_size, entry_set.info.count);
        node.Find(buffer, virtual_address);
        R_UNLESS(node.GetIndex() >= 0, fs::ResultOutOfRange());

        /* Copy the data into entry. */
        const auto entry_index = node.GetIndex();
        const auto entry_offset = impl::GetBucketTreeEntryOffset(0, entry_size, entry_index);
        std::memcpy(this->entry, buffer + entry_offset, entry_size);

        /* Set our entry set/index. */
        this->entry_set   = entry_set;
        this->entry_index = entry_index;

        return ResultSuccess();
    }

    Result BucketTree::Visitor::FindEntryWithoutBuffer(s64 virtual_address, s32 entry_set_index) {
        /* Calculate entry set extents. */
        const auto entry_size       = this->tree->entry_size;
        const auto entry_set_size   = this->tree->node_size;
        const auto entry_set_offset = entry_set_index * static_cast<s64>(entry_set_size);
        fs::SubStorage &storage = tree->entry_storage;

        /* Read and validate the entry_set. */
        EntrySetHeader entry_set;
        R_TRY(storage.Read(entry_set_offset, std::addressof(entry_set), sizeof(EntrySetHeader)));
        R_TRY(entry_set.header.Verify(entry_set_index, entry_set_size, entry_size));

        /* Create the node, and find. */
        StorageNode node(entry_set_offset, entry_size, entry_set.info.count);
        R_TRY(node.Find(storage, virtual_address));
        R_UNLESS(node.GetIndex() >= 0, fs::ResultOutOfRange());

        /* Copy the data into entry. */
        const auto entry_index = node.GetIndex();
        const auto entry_offset = impl::GetBucketTreeEntryOffset(entry_set_offset, entry_size, entry_index);
        R_TRY(storage.Read(entry_offset, this->entry, entry_size));

        /* Set our entry set/index. */
        this->entry_set   = entry_set;
        this->entry_index = entry_index;

        return ResultSuccess();
    }

}
