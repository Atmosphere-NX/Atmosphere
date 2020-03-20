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
#include <vapours/common.hpp>
#include <vapours/assert.hpp>
#include <vapours/util/util_typed_storage.hpp>

namespace ams::util {

    template<class Key, class Value, size_t N>
    class BoundedMap {
        private:
            std::array<std::optional<Key>, N> keys;
            std::array<TYPED_STORAGE(Value), N> values;
        private:
            ALWAYS_INLINE void FreeEntry(size_t i) {
                this->keys[i].reset();
                GetReference(this->values[i]).~Value();
            }
        public:
            constexpr BoundedMap() : keys(), values() { /* ... */ }

            Value *Find(const Key &key) {
                for (size_t i = 0; i < N; i++) {
                    if (this->keys[i] && this->keys[i].value() == key) {
                        return GetPointer(this->values[i]);
                    }
                }
                return nullptr;
            }

            void Remove(const Key &key) {
                for (size_t i = 0; i < N; i++) {
                    if (this->keys[i] && this->keys[i].value() == key) {
                        this->FreeEntry(i);
                        break;
                    }
                }
            }

            void RemoveAll() {
                for (size_t i = 0; i < N; i++) {
                    this->FreeEntry(i);
                }
            }

            bool IsFull() {
                for (size_t i = 0; i < N; i++) {
                    if (!this->keys[i]) {
                        return false;
                    }
                }

                return true;
            }

            bool Insert(const Key &key, Value &&value) {
                /* We can't insert if the key is used. */
                if (this->Find(key) != nullptr) {
                    return false;
                }

                /* Find a free value. */
                for (size_t i = 0; i < N; i++) {
                    if (!this->keys[i]) {
                        this->keys[i] = key;
                        new (GetPointer(this->values[i])) Value(std::move(value));
                        return true;
                    }
                }

                return false;
            }

            bool InsertOrAssign(const Key &key, Value &&value) {
                /* Try to find and assign an existing value. */
                for (size_t i = 0; i < N; i++) {
                    if (this->keys[i] && this->keys[i].value() == key) {
                        GetReference(this->values[i]) = std::move(value);
                        return true;
                    }
                }

                /* Find a free value. */
                for (size_t i = 0; i < N; i++) {
                    if (!this->keys[i]) {
                        this->keys[i] = key;
                        new (GetPointer(this->values[i])) Value(std::move(value));
                        return true;
                    }
                }

                return false;
            }

            template<class... Args>
            bool Emplace(const Key &key, Args&&... args) {
                /* We can't emplace if the key is used. */
                if (this->Find(key) != nullptr) {
                    return false;
                }

                /* Find a free value. */
                for (size_t i = 0; i < N; i++) {
                    if (!this->keys[i]) {
                        this->keys[i] = key;
                        new (GetPointer(this->values[i])) Value(std::forward<Args>(args)...);
                        return true;
                    }
                }

                return false;
            }
    };

}
