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
#include <stratosphere/fssystem/fssystem_dbm_rom_types.hpp>

namespace ams::fssystem {

    constexpr ALWAYS_INLINE u32 AlignRomAddress(u32 addr) {
        return util::AlignUp(addr, sizeof(addr));
    }

    template<typename BucketStorageType, typename EntryStorageType, typename KeyType, typename ValueType, size_t MaxAuxiliarySize>
    class RomKeyValueStorage {
        public:
            using BucketStorage = BucketStorageType;
            using EntryStorage  = EntryStorageType;
            using Key           = KeyType;
            using Value         = ValueType;
            using Position      = u32;
            using BucketIndex   = u32;

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
            s64 bucket_offset;
            u32 bucket_count;
            BucketStorage *bucket_storage;
            s64 kv_offset;
            u32 kv_size;
            EntryStorage *kv_storage;
            u32 total_entry_size;
            u32 entry_count;
        public:
            static constexpr u32 QueryEntrySize(u32 aux_size) {
                return AlignRomAddress(sizeof(Element) + aux_size);
            }

            static constexpr u32 QueryBucketStorageSize(u32 num) {
                return num * sizeof(Position);
            }

            static constexpr u32 QueryBucketCount(u32 size) {
                return size / sizeof(Position);
            }

            static constexpr u32 QueryKeyValueStorageSize(u32 num) {
                return num * sizeof(Element);
            }

            static Result Format(BucketStorage *bucket, s64 bucket_ofs, u32 bucket_count, EntryStorage *kv, s64 kv_ofs, u32 kv_size) {
                AMS_ASSERT(bucket != nullptr);
                AMS_ASSERT(kv != nullptr);
                AMS_ASSERT(kv_size >= 0);

                const Position pos = InvalidPosition;
                for (s64 i = 0; i < bucket_count; i++) {
                    R_TRY(bucket->Write(bucket_ofs + i * sizeof(pos), std::addressof(pos), sizeof(pos)));
                }
                return ResultSuccess();
            }
        public:
            RomKeyValueStorage() : bucket_offset(), bucket_count(), bucket_storage(), kv_offset(), kv_size(), kv_storage(), total_entry_size(), entry_count() { /* ... */ }

            Result Initialize(BucketStorage *bucket, s64 bucket_ofs, u32 bucket_count, EntryStorage *kv, s64 kv_ofs, u32 kv_size) {
                AMS_ASSERT(bucket != nullptr);
                AMS_ASSERT(kv != nullptr);
                AMS_ASSERT(bucket_count > 0);

                this->bucket_storage = bucket;
                this->bucket_offset  = bucket_ofs;
                this->bucket_count   = bucket_count;

                this->kv_storage     = kv;
                this->kv_offset      = kv_ofs;
                this->kv_size        = kv_size;

                return ResultSuccess();
            }

            void Finalize() {
                this->bucket_storage = nullptr;
                this->bucket_offset  = 0;
                this->bucket_count   = 0;

                this->kv_storage     = nullptr;
                this->kv_offset      = 0;
                this->kv_size        = 0;
            }

            constexpr u32 GetTotalEntrySize() const {
                return this->total_entry_size;
            }

            constexpr u32 GetFreeSize() const {
                return (this->kv_size - this->total_entry_size);
            }

            constexpr u32 GetEntryCount() const {
                return this->entry_count;
            }

            Result Add(const Key &key, u32 hash_key, const void *aux, size_t aux_size, const Value &value) {
                AMS_ASSERT(aux != nullptr);
                AMS_ASSERT(aux_size <= MaxAuxiliarySize);
                Position pos;
                return this->AddImpl(std::addressof(pos), key, hash_key, aux, aux_size, value);
            }

            Result Get(Value *out, const Key &key, u32 hash_key, const void *aux, size_t aux_size) {
                AMS_ASSERT(aux != nullptr);
                AMS_ASSERT(aux_size <= MaxAuxiliarySize);
                Position pos;
                return this->GetImpl(std::addressof(pos), out, key, hash_key, aux, aux_size);
            }

            void FindOpen(FindIndex *out) const {
                AMS_ASSERT(out != nullptr);

                out->ind = static_cast<BucketIndex>(-1);
                out->pos = InvalidPosition;
            }

            Result FindNext(Key *out_key, Value *out_val, FindIndex *find) {
                AMS_ASSERT(out_key != nullptr);
                AMS_ASSERT(out_val != nullptr);
                AMS_ASSERT(find != nullptr);

                Element elem;

                BucketIndex ind = find->ind;
                R_UNLESS((ind < this->bucket_count) || ind == static_cast<BucketIndex>(-1), fs::ResultDbmFindKeyFinished());

                while (true) {
                    if (find->pos != InvalidPosition) {
                        R_TRY(this->ReadKeyValue(std::addressof(elem), nullptr, nullptr, find->pos));

                        AMS_ASSERT(elem.next == InvalidPosition || elem.next < this->kv_size);
                        find->pos = elem.next;
                        *out_key = elem.key;
                        *out_val = elem.val;
                        return ResultSuccess();
                    }

                    while (true) {
                        ind++;
                        if (ind == this->bucket_count) {
                            find->ind = ind;
                            find->pos = InvalidPosition;
                            return fs::ResultDbmFindKeyFinished();
                        }

                        Position pos;
                        R_TRY(this->ReadBucket(std::addressof(pos), ind));
                        AMS_ASSERT(pos == InvalidPosition || pos < this->kv_size);

                        if (pos != InvalidPosition) {
                            find->ind = ind;
                            find->pos = pos;
                            break;
                        }
                    }
                }
            }
        protected:
            Result AddImpl(Position *out, const Key &key, u32 hash_key, const void *aux, size_t aux_size, const Value &value) {
                Position pos, prev_pos;
                Element elem;

                AMS_ASSERT(out != nullptr);
                AMS_ASSERT(this->bucket_count > 0);
                AMS_ASSERT(this->kv_size >= 0);

                const Result find_res = this->FindImpl(std::addressof(pos), std::addressof(prev_pos), std::addressof(elem), key, hash_key, aux, aux_size);
                R_UNLESS(R_FAILED(find_res),                           fs::ResultDbmAlreadyExists());
                R_UNLESS(fs::ResultDbmKeyNotFound::Includes(find_res), find_res);

                R_TRY(this->AllocateEntry(std::addressof(pos), aux_size));

                Position next_pos;
                R_TRY(this->LinkEntry(std::addressof(next_pos), pos, hash_key));

                elem = { key, value, next_pos, static_cast<u32>(aux_size) };
                *out = pos;
                R_TRY(this->WriteKeyValue(std::addressof(elem), pos, aux, aux_size, fs::WriteOption::None));

                this->entry_count++;

                return ResultSuccess();
            }

            Result GetImpl(Position *out_pos, Value *out_val, const Key &key, u32 hash_key, const void *aux, size_t aux_size) const {
                Position pos, prev_pos;
                Element elem;

                AMS_ASSERT(out_pos != nullptr);
                AMS_ASSERT(out_val != nullptr);

                R_TRY(this->FindImpl(std::addressof(pos), std::addressof(prev_pos), std::addressof(elem), key, hash_key, aux, aux_size));

                *out_pos = pos;
                *out_val = elem.value;
                return ResultSuccess();
            }

            Result GetByPosition(Key *out_key, Value *out_val, void *out_aux, size_t *out_aux_size, Position pos) const {
                AMS_ASSERT(out_key != nullptr);
                AMS_ASSERT(out_val != nullptr);

                Element elem;
                R_TRY(this->ReadKeyValue(std::addressof(elem), out_aux, out_aux_size, pos));

                *out_key = elem.key;
                *out_val = elem.value;
                return ResultSuccess();
            }

            Result SetByPosition(Position pos, const Value &value, fs::WriteOption option) const {
                Element elem;
                R_TRY(this->ReadKeyValue(std::addressof(elem), nullptr, nullptr, pos));
                elem.value = value;
                return this->WriteKeyValue(std::addressof(elem), pos, nullptr, 0, option);
            }
        private:
            BucketIndex HashToBucket(u32 hash_key) const {
                return hash_key % this->bucket_count;
            }

            Result FindImpl(Position *out_pos, Position *out_prev, Element *out_elem, const Key &key, u32 hash_key, const void *aux, size_t aux_size) const {
                AMS_ASSERT(out_pos != nullptr);
                AMS_ASSERT(out_prev != nullptr);
                AMS_ASSERT(out_elem != nullptr);
                AMS_ASSERT(this->bucket_count > 0);
                AMS_ASSERT(this->kv_size >= 0);

                *out_pos = 0;
                *out_prev = 0;

                const BucketIndex ind = HashToBucket(hash_key);

                Position cur;
                R_TRY(this->ReadBucket(std::addressof(cur), ind));
                AMS_ASSERT(cur == InvalidPosition || cur < this->kv_size);

                R_UNLESS(cur != InvalidPosition, fs::ResultDbmKeyNotFound());

                u8 buf[MaxAuxiliarySize];

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

                R_UNLESS(this->total_entry_size + sizeof(Element) + aux_size <= this->kv_size, fs::ResultDbmKeyFull());

                *out = static_cast<Position>(this->total_entry_size);

                this->total_entry_size = AlignRomAddress(this->total_entry_size + sizeof(Element) + static_cast<u32>(aux_size));
                return ResultSuccess();
            }

            Result LinkEntry(Position *out, Position pos, u32 hash_key) {
                AMS_ASSERT(out != nullptr);

                const BucketIndex ind = HashToBucket(hash_key);

                Position next;
                R_TRY(this->ReadBucket(std::addressof(next), ind));
                AMS_ASSERT(next == InvalidPosition || next < this->kv_size);

                R_TRY(this->WriteBucket(pos, ind, fs::WriteOption::None));

                *out = next;
                return ResultSuccess();
            }

            Result ReadBucket(Position *out, BucketIndex ind) const {
                AMS_ASSERT(out != nullptr);
                AMS_ASSERT(this->bucket_storage != nullptr);
                AMS_ASSERT(ind < this->bucket_count);

                const s64 offset = this->bucket_offset + ind * sizeof(Position);
                return this->bucket_storage->Read(offset, out, sizeof(*out));
            }

            Result WriteBucket(Position pos, BucketIndex ind, fs::WriteOption option) const {
                AMS_ASSERT(this->bucket_storage != nullptr);
                AMS_ASSERT(ind < this->bucket_count);

                const s64 offset = this->bucket_offset + ind * sizeof(Position);
                return this->bucket_storage.Write(offset, std::addressof(pos), sizeof(pos));
            }

            Result ReadKeyValue(Element *out, void *out_aux, size_t *out_aux_size, Position pos) const {
                AMS_ASSERT(out != nullptr);
                AMS_ASSERT(this->kv_storage != nullptr);
                AMS_ASSERT(pos < this->kv_size);

                const s64 offset = this->kv_offset + pos;
                R_TRY(this->kv_storage->Read(offset, out, sizeof(*out)));

                if (out_aux != nullptr && out_aux_size != nullptr) {
                    *out_aux_size = out->size;
                    if (out->size > 0) {
                        R_TRY(this->kv_storage->Read(offset + sizeof(*out), out_aux, out->size));
                    }
                }

                return ResultSuccess();
            }

            Result WriteKeyValue(const Element *elem, Position pos, const void *aux, size_t aux_size, fs::WriteOption option) const {
                AMS_ASSERT(elem != nullptr);
                AMS_ASSERT(this->kv_storage != nullptr);
                AMS_ASSERT(pos < this->kv_size);

                const s64 offset = this->kv_offset + pos;
                R_TRY(this->kv_storage->Write(offset, elem, sizeof(*elem)));

                if (aux != nullptr && aux_size > 0) {
                    R_TRY(this->kv_storage->Write(offset + sizeof(*elem), aux, aux_size));
                }

                return ResultSuccess();
            }
    };

}
