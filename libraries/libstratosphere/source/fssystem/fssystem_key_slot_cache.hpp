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

namespace ams::fssystem {

    class KeySlotCacheAccessor : public ::ams::fs::impl::Newable {
        NON_COPYABLE(KeySlotCacheAccessor);
        NON_MOVEABLE(KeySlotCacheAccessor);
        private:
            std::unique_lock<os::Mutex> lk;
            const s32 slot_index;
        public:
            KeySlotCacheAccessor(s32 idx, std::unique_lock<os::Mutex> &&l) : lk(std::move(l)), slot_index(idx) { /* ... */ }

            s32 GetKeySlotIndex() const { return this->slot_index; }
    };

    class KeySlotCacheEntry : public util::IntrusiveListBaseNode<KeySlotCacheEntry> {
        NON_COPYABLE(KeySlotCacheEntry);
        NON_MOVEABLE(KeySlotCacheEntry);
        public:
            static constexpr size_t KeySize = crypto::AesDecryptor128::KeySize;
        private:
            const s32 slot_index;
            u8 key1[KeySize];
            s32 key2;
        public:
            explicit KeySlotCacheEntry(s32 idx) : slot_index(idx), key2(-1) {
                std::memset(this->key1, 0, sizeof(this->key1));
            }

            bool Contains(const void *key, size_t key_size, s32 key2) const {
                AMS_ASSERT(key_size == KeySize);
                return key2 == this->key2 && std::memcmp(this->key1, key, KeySize) == 0;
            }

            s32 GetKeySlotIndex() const { return this->slot_index; }

            void SetKey(const void *key, size_t key_size, s32 key2) {
                AMS_ASSERT(key_size == KeySize);
                std::memcpy(this->key1, key, key_size);
                this->key2 = key2;
            }
    };

    class KeySlotCache {
        NON_COPYABLE(KeySlotCache);
        NON_MOVEABLE(KeySlotCache);
        private:
            using KeySlotCacheEntryList = util::IntrusiveListBaseTraits<KeySlotCacheEntry>::ListType;
        private:
            os::Mutex mutex;
            KeySlotCacheEntryList high_priority_mru_list;
            KeySlotCacheEntryList low_priority_mru_list;
        public:
            constexpr KeySlotCache() : mutex(false), high_priority_mru_list(), low_priority_mru_list() { /* ... */ }

            Result AllocateHighPriority(std::unique_ptr<KeySlotCacheAccessor> *out, const void *key, size_t key_size, s32 key2) {
                return this->AllocateFromLru(out, this->high_priority_mru_list, key, key_size, key2);
            }

            Result AllocateLowPriority(std::unique_ptr<KeySlotCacheAccessor> *out, const void *key, size_t key_size, s32 key2) {
                return this->AllocateFromLru(out, this->high_priority_mru_list, key, key_size, key2);
            }

            Result Find(std::unique_ptr<KeySlotCacheAccessor> *out, const void *key, size_t key_size, s32 key2) {
                std::unique_lock lk(this->mutex);

                KeySlotCacheEntryList *lists[2] = { std::addressof(this->high_priority_mru_list), std::addressof(this->low_priority_mru_list) };
                for (auto list : lists) {
                    for (auto it = list->begin(); it != list->end(); ++it) {
                        if (it->Contains(key, key_size, key2)) {
                            std::unique_ptr accessor = std::make_unique<KeySlotCacheAccessor>(it->GetKeySlotIndex(), std::move(lk));
                            R_UNLESS(accessor != nullptr, fs::ResultAllocationFailure());

                            *out = std::move(accessor);

                            this->UpdateMru(list, it);
                            return ResultSuccess();
                        }
                    }
                }

                return fs::ResultTargetNotFound();
            }

            void AddEntry(KeySlotCacheEntry *entry) {
                std::unique_lock lk(this->mutex);
                this->low_priority_mru_list.push_front(*entry);
            }
        private:
            Result AllocateFromLru(std::unique_ptr<KeySlotCacheAccessor> *out, KeySlotCacheEntryList &dst_list, const void *key, size_t key_size, s32 key2) {
                std::unique_lock lk(this->mutex);

                KeySlotCacheEntryList &src_list = this->low_priority_mru_list.empty() ? this->high_priority_mru_list : this->low_priority_mru_list;
                AMS_ASSERT(!src_list.empty());

                auto it = src_list.rbegin();
                std::unique_ptr accessor = std::make_unique<KeySlotCacheAccessor>(it->GetKeySlotIndex(), std::move(lk));
                *out = std::move(accessor);

                it->SetKey(key, key_size, key2);

                auto *entry = std::addressof(*it);
                src_list.pop_back();
                dst_list.push_front(*entry);

                return ResultSuccess();
            }

            void UpdateMru(KeySlotCacheEntryList *list, KeySlotCacheEntryList::iterator it) {
                auto *entry = std::addressof(*it);
                list->erase(it);
                list->push_front(*entry);
            }
    };

}
