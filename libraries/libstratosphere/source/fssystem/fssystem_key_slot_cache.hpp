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

namespace ams::fssystem {

    class KeySlotCacheAccessor : public ::ams::fs::impl::Newable {
        NON_COPYABLE(KeySlotCacheAccessor);
        NON_MOVEABLE(KeySlotCacheAccessor);
        private:
            util::unique_lock<os::SdkMutex> m_lk;
            const s32 m_slot_index;
        public:
            KeySlotCacheAccessor(s32 idx, util::unique_lock<os::SdkMutex> &&l) : m_lk(std::move(l)), m_slot_index(idx) { /* ... */ }

            s32 GetKeySlotIndex() const { return m_slot_index; }
    };

    class KeySlotCacheEntry : public util::IntrusiveListBaseNode<KeySlotCacheEntry> {
        NON_COPYABLE(KeySlotCacheEntry);
        NON_MOVEABLE(KeySlotCacheEntry);
        public:
            static constexpr size_t KeySize = crypto::AesDecryptor128::KeySize;
        private:
            const s32 m_slot_index;
            u8 m_key1[KeySize];
            s32 m_key2;
        public:
            explicit KeySlotCacheEntry(s32 idx) : m_slot_index(idx), m_key2(-1) {
                std::memset(m_key1, 0, sizeof(m_key1));
            }

            bool Contains(const void *key, size_t key_size, s32 key2) const {
                AMS_ASSERT(key_size == KeySize);
                AMS_UNUSED(key_size);

                return key2 == m_key2 && std::memcmp(m_key1, key, KeySize) == 0;
            }

            s32 GetKeySlotIndex() const { return m_slot_index; }

            void SetKey(const void *key, size_t key_size, s32 key2) {
                AMS_ASSERT(key_size == KeySize);
                std::memcpy(m_key1, key, key_size);
                m_key2 = key2;
            }
    };

    class KeySlotCache {
        NON_COPYABLE(KeySlotCache);
        NON_MOVEABLE(KeySlotCache);
        private:
            using KeySlotCacheEntryList = util::IntrusiveListBaseTraits<KeySlotCacheEntry>::ListType;
        private:
            os::SdkMutex m_mutex;
            KeySlotCacheEntryList m_high_priority_mru_list;
            KeySlotCacheEntryList m_low_priority_mru_list;
        public:
            constexpr KeySlotCache() : m_mutex(), m_high_priority_mru_list(), m_low_priority_mru_list() { /* ... */ }

            Result AllocateHighPriority(std::unique_ptr<KeySlotCacheAccessor> *out, const void *key, size_t key_size, s32 key2) {
                return this->AllocateFromLru(out, m_high_priority_mru_list, key, key_size, key2);
            }

            Result AllocateLowPriority(std::unique_ptr<KeySlotCacheAccessor> *out, const void *key, size_t key_size, s32 key2) {
                return this->AllocateFromLru(out, m_high_priority_mru_list, key, key_size, key2);
            }

            Result Find(std::unique_ptr<KeySlotCacheAccessor> *out, const void *key, size_t key_size, s32 key2) {
                util::unique_lock lk(m_mutex);

                KeySlotCacheEntryList *lists[2] = { std::addressof(m_high_priority_mru_list), std::addressof(m_low_priority_mru_list) };
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
                util::unique_lock lk(m_mutex);
                m_low_priority_mru_list.push_front(*entry);
            }
        private:
            Result AllocateFromLru(std::unique_ptr<KeySlotCacheAccessor> *out, KeySlotCacheEntryList &dst_list, const void *key, size_t key_size, s32 key2) {
                util::unique_lock lk(m_mutex);

                KeySlotCacheEntryList &src_list = m_low_priority_mru_list.empty() ? m_high_priority_mru_list : m_low_priority_mru_list;
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
