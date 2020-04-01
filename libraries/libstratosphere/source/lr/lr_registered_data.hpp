/*
 * Copyright (c) 2019-2020 Adubbz, Atmosphère-NX
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
#include <stratosphere/lr/lr_types.hpp>

namespace ams::lr {

    template<typename Key, typename Value, size_t NumEntries>
    class RegisteredData {
        NON_COPYABLE(RegisteredData);
        NON_MOVEABLE(RegisteredData);
        private:
            struct Entry {
                Value value;
                ncm::ProgramId owner_id;
                Key key;
                bool is_valid;
            };
        private:
            Entry entries[NumEntries];
            size_t capacity;
        private:
            inline bool IsExcluded(const ncm::ProgramId id, const ncm::ProgramId *excluding_ids, size_t num_ids) const {
                /* Try to find program id in exclusions. */
                for (size_t i = 0; i < num_ids; i++) {
                    if (id == excluding_ids[i]) {
                        return true;
                    }
                }

                return false;
            }

            inline void RegisterImpl(size_t i, const Key &key, const Value &value, const ncm::ProgramId owner_id) {
                /* Populate entry. */
                Entry &entry = this->entries[i];
                entry.key = key;
                entry.value = value;
                entry.owner_id = owner_id;
                entry.is_valid = true;
            }
        public:
            RegisteredData(size_t capacity = NumEntries) : capacity(capacity) {
                this->Clear();
            }

            bool Register(const Key &key, const Value &value, const ncm::ProgramId owner_id) {
                /* Try to find an existing value. */
                for (size_t i = 0; i < this->GetCapacity(); i++) {
                    Entry &entry = this->entries[i];
                    if (entry.is_valid && entry.key == key) {
                        this->RegisterImpl(i, key, value, owner_id);
                        return true;
                    }
                }

                /* We didn't find an existing entry, so try to create a new one. */
                for (size_t i = 0; i < this->GetCapacity(); i++) {
                    Entry &entry = this->entries[i];
                    if (!entry.is_valid) {
                        this->RegisterImpl(i, key, value, owner_id);
                        return true;
                    }
                }

                return false;
            }

            void Unregister(const Key &key) {
                /* Invalidate entries with a matching key. */
                for (size_t i = 0; i < this->GetCapacity(); i++) {
                    Entry &entry = this->entries[i];
                    if (entry.is_valid && entry.key == key) {
                        entry.is_valid = false;
                    }
                }
            }

            void UnregisterOwnerProgram(ncm::ProgramId owner_id) {
                /* Invalidate entries with a matching owner id. */
                for (size_t i = 0; i < this->GetCapacity(); i++) {
                    Entry &entry = this->entries[i];
                    if (entry.owner_id == owner_id) {
                        entry.is_valid = false;
                    }
                }
            }

            bool Find(Value *out, const Key &key) const {
                /* Locate a matching entry. */
                for (size_t i = 0; i < this->GetCapacity(); i++) {
                    const Entry &entry = this->entries[i];
                    if (entry.is_valid && entry.key == key) {
                        *out = entry.value;
                        return true;
                    }
                }

                return false;
            }

            void Clear() {
                /* Invalidate all entries. */
                for (size_t i = 0; i < this->GetCapacity(); i++) {
                    this->entries[i].is_valid = false;
                }
            }

            void ClearExcluding(const ncm::ProgramId *ids, size_t num_ids) {
                /* Invalidate all entries unless excluded. */
                for (size_t i = 0; i < this->GetCapacity(); i++) {
                    Entry &entry = this->entries[i];

                    if (!this->IsExcluded(entry.owner_id, ids, num_ids)) {
                        entry.is_valid = false;
                    }
                }
            }

            size_t GetCapacity() const {
                return this->capacity;
            }
    };

    template<typename Key, size_t NumEntries>
    using RegisteredLocations = RegisteredData<Key, lr::Path, NumEntries>;

    template<typename Key, size_t NumEntries>
    using RegisteredStorages = RegisteredData<Key, ncm::StorageId, NumEntries>;

}
