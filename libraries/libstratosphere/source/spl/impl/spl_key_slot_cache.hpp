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

namespace ams::spl::impl {

    class AesKeySlotCacheEntry : public util::IntrusiveListBaseNode<AesKeySlotCacheEntry> {
        NON_COPYABLE(AesKeySlotCacheEntry);
        NON_MOVEABLE(AesKeySlotCacheEntry);
        private:
            friend class AesKeySlotCache;
        public:
            static constexpr size_t KeySize = crypto::AesDecryptor128::KeySize;
        private:
            const s32 m_aes_keyslot_index;
            s32 m_virtual_aes_keyslot;
        public:
            explicit AesKeySlotCacheEntry(s32 idx) : m_aes_keyslot_index(idx), m_virtual_aes_keyslot(-1) { /* ... */ }

            bool Contains(s32 virtual_slot) const {
                return virtual_slot == m_virtual_aes_keyslot;
            }

            s32 GetPhysicalAesKeySlotIndex() const { return m_aes_keyslot_index; }

            s32 GetVirtualAesKeySlotIndex() const { return m_virtual_aes_keyslot; }

            void SetVirtualAesKeySlot(s32 virtual_slot) {
                m_virtual_aes_keyslot = virtual_slot;
            }

            void ClearVirtualAesKeySlot() {
                m_virtual_aes_keyslot = -1;
            }
    };

    class AesKeySlotCache {
        NON_COPYABLE(AesKeySlotCache);
        NON_MOVEABLE(AesKeySlotCache);
        private:
            using AesKeySlotCacheEntryList = util::IntrusiveListBaseTraits<AesKeySlotCacheEntry>::ListType;
        private:
            AesKeySlotCacheEntryList m_mru_list;
        public:
            constexpr AesKeySlotCache() : m_mru_list() { /* ... */ }

            s32 Allocate(s32 virtual_slot) {
                return this->AllocateFromLru(virtual_slot);
            }

            bool Find(s32 *out, s32 virtual_slot) {
                for (auto it = m_mru_list.begin(); it != m_mru_list.end(); ++it) {
                    if (it->Contains(virtual_slot)) {
                        *out = it->GetPhysicalAesKeySlotIndex();

                        this->UpdateMru(it);
                        return true;
                    }
                }

                return false;
            }

            bool Release(s32 *out, s32 virtual_slot) {
                for (auto it = m_mru_list.begin(); it != m_mru_list.end(); ++it) {
                    if (it->Contains(virtual_slot)) {
                        *out = it->GetPhysicalAesKeySlotIndex();
                        it->ClearVirtualAesKeySlot();

                        this->UpdateLru(it);
                        return true;
                    }
                }

                return false;
            }

            bool FindPhysical(s32 physical_slot) {
                for (auto it = m_mru_list.begin(); it != m_mru_list.end(); ++it) {
                    if (it->GetPhysicalAesKeySlotIndex() == physical_slot) {
                        this->UpdateMru(it);

                        if (it->GetVirtualAesKeySlotIndex() == physical_slot) {
                            return true;
                        } else {
                            it->SetVirtualAesKeySlot(physical_slot);
                            return false;
                        }
                    }
                }
                AMS_ABORT();
            }

            void AddEntry(AesKeySlotCacheEntry *entry) {
                m_mru_list.push_front(*entry);
            }
        private:
            s32 AllocateFromLru(s32 virtual_slot) {
                AMS_ASSERT(!m_mru_list.empty());

                auto it = m_mru_list.rbegin();
                it->SetVirtualAesKeySlot(virtual_slot);

                auto *entry = std::addressof(*it);
                m_mru_list.pop_back();
                m_mru_list.push_front(*entry);

                return entry->GetPhysicalAesKeySlotIndex();
            }

            void UpdateMru(AesKeySlotCacheEntryList::iterator it) {
                auto *entry = std::addressof(*it);
                m_mru_list.erase(it);
                m_mru_list.push_front(*entry);
            }

            void UpdateLru(AesKeySlotCacheEntryList::iterator it) {
                auto *entry = std::addressof(*it);
                m_mru_list.erase(it);
                m_mru_list.push_back(*entry);
            }
    };

}
