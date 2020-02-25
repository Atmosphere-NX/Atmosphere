/*
 * Copyright (c) 2019-2020 Adubbz, Atmosphere-NX
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

namespace ams::ncm::impl {
    
    template<class Key, class Value, size_t N>
    class BoundedMap {
        private:
            std::array<std::optional<Key>, N> keys;
            std::array<Value, N> values;
        public:
            Value *Find(const Key &key) {
                for (size_t i = 0; i < N; i++) {
                    if (this->keys[i] && this->keys[i].value() == key) {
                        return &this->values[i];
                    }
                }
                return nullptr;
            }

            void Remove(const Key &key) {
                for (size_t i = 0; i < N; i++) {
                    if (this->keys[i] && this->keys[i].value() == key) {
                        this->keys[i].reset();
                    }
                }
            }

            void RemoveAll() {
                for (size_t i = 0; i < N; i++) {
                    this->keys[i].reset();
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

            Value &operator[](const Key &key) {
                /* Try to find an existing value. */
                {
                    Value *value = this->Find(key);
                    if (value) {
                        return *value;
                    }
                }

                /* Reference a new value. */
                for (size_t i = 0; i < N; i++) {
                    if (!this->keys[i]) {
                        this->keys[i] = key;
                        return this->values[i];
                    }
                }

                /* We ran out of space in the map. */
                std::abort();
            }
    };

}
