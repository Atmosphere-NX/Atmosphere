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

namespace ams::i2c::driver::board::nintendo_nx::impl {

    template<typename ListType>
    class IAllocator {
        private:
            using T = typename ListType::value_type;
        private:
            ams::MemoryResource *memory_resource;
            ListType list;
            mutable os::SdkMutex list_lock;
        public:
            IAllocator(ams::MemoryResource *mr) : memory_resource(mr), list(), list_lock() { /* ... */ }

            ~IAllocator() {
                std::scoped_lock lk(this->list_lock);

                /* Remove all entries. */
                auto it = this->list.begin();
                while (it != this->list.end()) {
                    T *obj = std::addressof(*it);
                    it = this->list.erase(it);

                    obj->~T();
                    this->memory_resource->Deallocate(obj, sizeof(T));
                }
            }

            template<typename ...Args>
            T *Allocate(Args &&...args) {
                std::scoped_lock lk(this->list_lock);

                /* Allocate space for the object. */
                void *storage = this->memory_resource->Allocate(sizeof(T), alignof(T));
                AMS_ABORT_UNLESS(storage != nullptr);

                /* Construct the object. */
                T *t = new (static_cast<T *>(storage)) T(std::forward<Args>(args)...);

                /* Link the object into our list. */
                this->list.push_back(*t);

                return t;
            }

            template<typename F>
            T *Find(F f) {
                std::scoped_lock lk(this->list_lock);

                for (T &it : this->list) {
                    if (f(static_cast<const T &>(it))) {
                        return std::addressof(it);
                    }
                }

                return nullptr;
            }

            /* TODO: Support free */
    };

}
