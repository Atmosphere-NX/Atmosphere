/*
 * Copyright (c) 2019 Adubbz
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
#include <switch.h>
#include <stratosphere.hpp>

#include "../lr_types.hpp"

namespace sts::lr::impl {

    template<typename Key, typename Value, size_t NumEntries>
    class RegisteredData {
        NON_COPYABLE(RegisteredData);
        NON_MOVEABLE(RegisteredData);
        private:
            struct Entry {
                Value value;
                Key key;
                bool is_valid;
            };
        private:
            Entry entries[NumEntries];
        public:
            RegisteredData() {
                this->Clear();
            }

            bool Register(const Key &key, const Value &value) {
                /* Try to find an existing value. */
                for (size_t i = 0; i < NumEntries; i++) {
                    Entry& entry = this->entries[i];
                    if (entry.is_valid && entry.key == key) {
                        entry.value = value;
                        return true;
                    }
                }

                for (size_t i = 0; i < NumEntries; i++) {
                    Entry& entry = this->entries[i];
                    if (!entry.is_valid) {
                        entry.key = key;
                        entry.value = value;
                        entry.is_valid = true;
                        return true;
                    }
                }

                return false;
            }

            void Unregister(const Key &key) {
                for (size_t i = 0; i < NumEntries; i++) {
                    Entry& entry = this->entries[i];
                    if (entry.is_valid && entry.key == key) {
                        entry.is_valid = false;
                    }
                }
            }

            bool Find(Value *out, const Key &key) {
                for (size_t i = 0; i < NumEntries; i++) {
                    Entry& entry = this->entries[i];
                    if (entry.is_valid && entry.key == key) {
                        *out = entry.value;
                        return true;
                    }
                }

                return false;
            }

            void Clear() {
                for (size_t i = 0; i < NumEntries; i++) {
                    this->entries[i].is_valid = false;
                }
            }
    };

    template<typename Key, size_t NumEntries>
    using RegisteredLocations = RegisteredData<Key, lr::Path, NumEntries>;

    template<typename Key, size_t NumEntries>
    using RegisteredStorages = RegisteredData<Key, ncm::StorageId, NumEntries>;

}