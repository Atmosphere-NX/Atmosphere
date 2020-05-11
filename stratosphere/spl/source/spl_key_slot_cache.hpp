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

namespace ams::spl {

    class KeySlotCacheEntry : public util::IntrusiveListBaseNode<KeySlotCacheEntry> {
        NON_COPYABLE(KeySlotCacheEntry);
        NON_MOVEABLE(KeySlotCacheEntry);
        private:
            friend class KeySlotCache;
        public:
            static constexpr size_t KeySize = crypto::AesDecryptor128::KeySize;
        private:
            const s32 slot_index;
            s32 virtual_slot;
        public:
            explicit KeySlotCacheEntry(s32 idx) : slot_index(idx), virtual_slot(-1) { /* ... */ }

            bool Contains(s32 virtual_slot) const {
                return virtual_slot == this->virtual_slot;
            }

            s32 GetPhysicalKeySlotIndex() const { return this->slot_index; }

            s32 GetVirtualKeySlotIndex() const { return this->virtual_slot; }

            void SetVirtualSlot(s32 virtual_slot) {
                this->virtual_slot = virtual_slot;
            }

            void ClearVirtualSlot() {
                this->virtual_slot = -1;
            }
    };

    class KeySlotCache {
        NON_COPYABLE(KeySlotCache);
        NON_MOVEABLE(KeySlotCache);
        private:
            using KeySlotCacheEntryList = util::IntrusiveListBaseTraits<KeySlotCacheEntry>::ListType;
        private:
            KeySlotCacheEntryList mru_list;
        public:
            constexpr KeySlotCache() : mru_list() { /* ... */ }

            s32 Allocate(s32 virtual_slot) {
                return this->AllocateFromLru(virtual_slot);
            }

            bool Find(s32 *out, s32 virtual_slot) {
                for (auto it = this->mru_list.begin(); it != this->mru_list.end(); ++it) {
                    if (it->Contains(virtual_slot)) {
                        *out = it->GetPhysicalKeySlotIndex();

                        this->UpdateMru(it);
                        return true;
                    }
                }

                return false;
            }

            bool Release(s32 *out, s32 virtual_slot) {
                for (auto it = this->mru_list.begin(); it != this->mru_list.end(); ++it) {
                    if (it->Contains(virtual_slot)) {
                        *out = it->GetPhysicalKeySlotIndex();
                        it->ClearVirtualSlot();

                        this->UpdateLru(it);
                        return true;
                    }
                }

                return false;
            }

            bool FindPhysical(s32 physical_slot) {
                for (auto it = this->mru_list.begin(); it != this->mru_list.end(); ++it) {
                    if (it->GetPhysicalKeySlotIndex() == physical_slot) {
                        this->UpdateMru(it);

                        if (it->GetVirtualKeySlotIndex() == physical_slot) {
                            return true;
                        } else {
                            it->SetVirtualSlot(physical_slot);
                            return false;
                        }
                    }
                }
                AMS_ABORT();
            }

            void AddEntry(KeySlotCacheEntry *entry) {
                this->mru_list.push_front(*entry);
            }
        private:
            s32 AllocateFromLru(s32 virtual_slot) {
                AMS_ASSERT(!this->mru_list.empty());

                auto it = this->mru_list.rbegin();
                it->SetVirtualSlot(virtual_slot);

                auto *entry = std::addressof(*it);
                this->mru_list.pop_back();
                this->mru_list.push_front(*entry);

                return entry->GetPhysicalKeySlotIndex();
            }

            void UpdateMru(KeySlotCacheEntryList::iterator it) {
                auto *entry = std::addressof(*it);
                this->mru_list.erase(it);
                this->mru_list.push_front(*entry);
            }

            void UpdateLru(KeySlotCacheEntryList::iterator it) {
                auto *entry = std::addressof(*it);
                this->mru_list.erase(it);
                this->mru_list.push_back(*entry);
            }
    };

}
