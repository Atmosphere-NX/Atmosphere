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
#include <stratosphere/fs/common/fs_dbm_rom_types.hpp>
#include <stratosphere/fs/fs_substorage.hpp>

namespace ams::fs {

    template<typename KeyType, typename ValueType, size_t MaxAuxiliarySize>
    class KeyValueRomStorageTemplate {
        public:
            using Key         = KeyType;
            using Value       = ValueType;
            using Position    = u32;
            using BucketIndex = s64;

            struct FindIndex {
                BucketIndex ind;
                Position pos;
            };
            static_assert(util::is_pod<FindIndex>::value);
        private:
            static constexpr inline Position InvalidPosition = ~Position();

            struct Element {
                Key key;
                Value value;
                Position next;
                u32 size;
            };
            static_assert(util::is_pod<Element>::value);
        private:
            s64 bucket_count;
            SubStorage bucket_storage;
            SubStorage kv_storage;
            s64 total_entry_size;
            u32 entry_count;
        public:
            static constexpr s64 QueryBucketStorageSize(s64 num) {
                return num * sizeof(Position);
            }

            static constexpr s64 QueryBucketCount(s64 size) {
                return size / sizeof(Position);
            }

            static constexpr size_t QueryEntrySize(size_t aux_size) {
                return util::AlignUp(sizeof(Element) + aux_size, alignof(Element));
            }

            static Result Format(SubStorage bucket, s64 count) {
                const Position pos = InvalidPosition;
                for (s64 i = 0; i < count; i++) {
                    R_TRY(bucket.Write(i * sizeof(pos), std::addressof(pos), sizeof(pos)));
                }
                return ResultSuccess();
            }
        public:
            KeyValueRomStorageTemplate() : bucket_count(), bucket_storage(), kv_storage(), total_entry_size(), entry_count() { /* ... */ }

            Result Initialize(const SubStorage &bucket, s64 count, const SubStorage &kv) {
                AMS_ASSERT(count > 0);
                this->bucket_storage = bucket;
                this->bucket_count   = count;
                this->kv_storage     = kv;
                return ResultSuccess();
            }

            void Finalize() {
                this->bucket_storage = SubStorage();
                this->kv_storage     = SubStorage();
                this->bucket_count   = 0;
            }

            s64 GetTotalEntrySize() const {
                return this->total_entry_size;
            }

            Result GetFreeSize(s64 *out) {
                AMS_ASSERT(out != nullptr);
                s64 kv_size = 0;
                R_TRY(this->kv_storage.GetSize(std::addressof(kv_size)));
                *out = kv_size - this->total_entry_size;
                return ResultSuccess();
            }

            constexpr u32 GetEntryCount() const {
                return this->entry_count;
            }
        protected:
            Result AddInternal(Position *out, const Key &key, u32 hash_key, const void *aux, size_t aux_size, const Value &value) {
                AMS_ASSERT(out != nullptr);
                AMS_ASSERT(aux != nullptr || aux_size == 0);
                AMS_ASSERT(this->bucket_count > 0);

                {
                    Position pos, prev_pos;
                    Element elem;

                    const Result find_res = this->FindInternal(std::addressof(pos), std::addressof(prev_pos), std::addressof(elem), key, hash_key, aux, aux_size);
                    R_UNLESS(R_FAILED(find_res),                           fs::ResultDbmAlreadyExists());
                    R_UNLESS(fs::ResultDbmKeyNotFound::Includes(find_res), find_res);
                }

                Position pos;
                R_TRY(this->AllocateEntry(std::addressof(pos), aux_size));

                Position next_pos;
                R_TRY(this->LinkEntry(std::addressof(next_pos), pos, hash_key));

                const Element elem = { key, value, next_pos, static_cast<u32>(aux_size) };
                R_TRY(this->WriteKeyValue(std::addressof(elem), pos, aux, aux_size));

                *out = pos;
                this->entry_count++;

                return ResultSuccess();
            }

            Result GetInternal(Position *out_pos, Value *out_val, const Key &key, u32 hash_key, const void *aux, size_t aux_size) {
                AMS_ASSERT(out_pos != nullptr);
                AMS_ASSERT(out_val != nullptr);
                AMS_ASSERT(aux     != nullptr);

                Position pos, prev_pos;
                Element elem;
                R_TRY(this->FindInternal(std::addressof(pos), std::addressof(prev_pos), std::addressof(elem), key, hash_key, aux, aux_size));

                *out_pos = pos;
                *out_val = elem.value;
                return ResultSuccess();
            }

            Result GetByPosition(Key *out_key, Value *out_val, Position pos) {
                AMS_ASSERT(out_key != nullptr);
                AMS_ASSERT(out_val != nullptr);

                Element elem;
                R_TRY(this->ReadKeyValue(std::addressof(elem), pos));

                *out_key = elem.key;
                *out_val = elem.value;
                return ResultSuccess();
            }

            Result GetByPosition(Key *out_key, Value *out_val, void *out_aux, size_t *out_aux_size, Position pos) {
                AMS_ASSERT(out_key != nullptr);
                AMS_ASSERT(out_val != nullptr);
                AMS_ASSERT(out_aux != nullptr);
                AMS_ASSERT(out_aux_size != nullptr);

                Element elem;
                R_TRY(this->ReadKeyValue(std::addressof(elem), out_aux, out_aux_size, pos));

                *out_key = elem.key;
                *out_val = elem.value;
                return ResultSuccess();
            }

            Result SetByPosition(Position pos, const Value &value) {
                Element elem;
                R_TRY(this->ReadKeyValue(std::addressof(elem), pos));
                elem.value = value;
                return this->WriteKeyValue(std::addressof(elem), pos, nullptr, 0);
            }
        private:
            BucketIndex HashToBucket(u32 hash_key) const {
                return hash_key % this->bucket_count;
            }

            Result FindInternal(Position *out_pos, Position *out_prev, Element *out_elem, const Key &key, u32 hash_key, const void *aux, size_t aux_size) {
                AMS_ASSERT(out_pos != nullptr);
                AMS_ASSERT(out_prev != nullptr);
                AMS_ASSERT(out_elem != nullptr);
                AMS_ASSERT(aux != nullptr || aux_size == 0);
                AMS_ASSERT(this->bucket_count > 0);

                *out_pos = 0;
                *out_prev = 0;

                const BucketIndex ind = HashToBucket(hash_key);

                Position cur;
                R_TRY(this->ReadBucket(std::addressof(cur), ind));

                s64 kv_size;
                R_TRY(this->kv_storage.GetSize(std::addressof(kv_size)));
                AMS_ASSERT(cur == InvalidPosition || cur < kv_size);

                R_UNLESS(cur != InvalidPosition, fs::ResultDbmKeyNotFound());

                u8 *buf = static_cast<u8 *>(::ams::fs::impl::Allocate(MaxAuxiliarySize));
                R_UNLESS(buf != nullptr, fs::ResultAllocationFailureInDbmRomKeyValueStorage());
                ON_SCOPE_EXIT { ::ams::fs::impl::Deallocate(buf, MaxAuxiliarySize); };

                while (true) {
                    size_t cur_aux_size;
                    R_TRY(this->ReadKeyValue(out_elem, buf, std::addressof(cur_aux_size), cur));

                    if (key.IsEqual(out_elem->key, aux, aux_size, buf, cur_aux_size)) {
                        *out_pos = cur;
                        return ResultSuccess();
                    }

                    *out_prev = cur;
                    cur = out_elem->next;
                    R_UNLESS(cur != InvalidPosition, fs::ResultDbmKeyNotFound());
                }
            }

            Result AllocateEntry(Position *out, size_t aux_size) {
                AMS_ASSERT(out != nullptr);

                s64 kv_size;
                R_TRY(this->kv_storage.GetSize(std::addressof(kv_size)));
                const size_t end_pos = this->total_entry_size + sizeof(Element) + aux_size;
                R_UNLESS(end_pos <= static_cast<size_t>(kv_size), fs::ResultDbmKeyFull());

                *out = static_cast<Position>(this->total_entry_size);

                this->total_entry_size = util::AlignUp(static_cast<s64>(end_pos), alignof(Position));
                return ResultSuccess();
            }

            Result LinkEntry(Position *out, Position pos, u32 hash_key) {
                AMS_ASSERT(out != nullptr);

                const BucketIndex ind = HashToBucket(hash_key);

                Position next;
                R_TRY(this->ReadBucket(std::addressof(next), ind));

                s64 kv_size;
                R_TRY(this->kv_storage.GetSize(std::addressof(kv_size)));
                AMS_ASSERT(next == InvalidPosition || next < kv_size);

                R_TRY(this->WriteBucket(pos, ind));

                *out = next;
                return ResultSuccess();
            }

            Result ReadBucket(Position *out, BucketIndex ind) {
                AMS_ASSERT(out != nullptr);
                AMS_ASSERT(ind < this->bucket_count);

                const s64 offset = ind * sizeof(Position);
                return this->bucket_storage.Read(offset, out, sizeof(*out));
            }

            Result WriteBucket(Position pos, BucketIndex ind) {
                AMS_ASSERT(ind < this->bucket_count);

                const s64 offset = ind * sizeof(Position);
                return this->bucket_storage.Write(offset, std::addressof(pos), sizeof(pos));
            }

            Result ReadKeyValue(Element *out, Position pos) {
                AMS_ASSERT(out != nullptr);

                s64 kv_size;
                R_TRY(this->kv_storage.GetSize(std::addressof(kv_size)));
                AMS_ASSERT(pos < kv_size);

                return this->kv_storage.Read(pos, out, sizeof(*out));
            }

            Result ReadKeyValue(Element *out, void *out_aux, size_t *out_aux_size, Position pos) {
                AMS_ASSERT(out != nullptr);
                AMS_ASSERT(out_aux != nullptr);
                AMS_ASSERT(out_aux_size != nullptr);

                R_TRY(this->ReadKeyValue(out, pos));

                *out_aux_size = out->size;
                if (out->size > 0) {
                    R_TRY(this->kv_storage.Read(pos + sizeof(*out), out_aux, out->size));
                }

                return ResultSuccess();
            }

            Result WriteKeyValue(const Element *elem, Position pos, const void *aux, size_t aux_size) {
                AMS_ASSERT(elem != nullptr);
                AMS_ASSERT(aux != nullptr);

                s64 kv_size;
                R_TRY(this->kv_storage.GetSize(std::addressof(kv_size)));
                AMS_ASSERT(pos < kv_size);

                R_TRY(this->kv_storage.Write(pos, elem, sizeof(*elem)));

                if (aux != nullptr && aux_size > 0) {
                    R_TRY(this->kv_storage.Write(pos + sizeof(*elem), aux, aux_size));
                }

                return ResultSuccess();
            }
    };

}
