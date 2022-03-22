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

namespace ams::i2c::driver::board::nintendo::nx::impl {

    template<typename ListType>
    class IAllocator {
        NON_COPYABLE(IAllocator);
        NON_MOVEABLE(IAllocator);
        private:
            using T = typename ListType::value_type;
        private:
            ams::MemoryResource *m_memory_resource;
            ListType m_list;
            mutable os::SdkMutex m_list_lock;
        public:
            IAllocator(ams::MemoryResource *mr) : m_memory_resource(mr), m_list(), m_list_lock() { /* ... */ }

            ~IAllocator() {
                std::scoped_lock lk(m_list_lock);

                /* Remove all entries. */
                auto it = m_list.begin();
                while (it != m_list.end()) {
                    T *obj = std::addressof(*it);
                    it = m_list.erase(it);

                    std::destroy_at(obj);
                    m_memory_resource->Deallocate(obj, sizeof(T));
                }
            }

            template<typename ...Args>
            T *Allocate(Args &&...args) {
                std::scoped_lock lk(m_list_lock);

                /* Allocate space for the object. */
                void *storage = m_memory_resource->Allocate(sizeof(T), alignof(T));
                AMS_ABORT_UNLESS(storage != nullptr);

                /* Construct the object. */
                T *t = std::construct_at(static_cast<T *>(storage), std::forward<Args>(args)...);

                /* Link the object into our list. */
                m_list.push_back(*t);

                return t;
            }

            template<typename F>
            T *Find(F f) {
                std::scoped_lock lk(m_list_lock);

                for (T &it : m_list) {
                    if (f(static_cast<const T &>(it))) {
                        return std::addressof(it);
                    }
                }

                return nullptr;
            }

            /* TODO: Support free */
    };

}
