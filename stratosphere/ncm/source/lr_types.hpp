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

namespace sts::lr {

    constexpr size_t MaxPathLen = 0x300;

    struct Path {
        char path[MaxPathLen];

        Path() {
            path[0] = '\0';
        }

        Path(const char* path) {
            strlcpy(this->path, path, MaxPathLen);
        }

        Path& operator=(const Path& other) {
            /* N appears to always memcpy paths, so we will too. */
            memcpy(this->path, other.path, MaxPathLen);
            this->EnsureNullTerminated();
            return *this;
        }

        void EnsureNullTerminated() {
            const size_t len = strnlen(this->path, MaxPathLen);

            if (len == MaxPathLen) {
                path[MaxPathLen-1] = '\0';
            }
        }
    };

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

    static constexpr Result ResultLrDebugProgramNotFound              = MAKERESULT(Module_Lr, 10);

}