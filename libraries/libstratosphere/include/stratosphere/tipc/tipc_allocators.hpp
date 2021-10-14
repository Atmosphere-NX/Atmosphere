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
#include <vapours.hpp>
#include <stratosphere/tipc/tipc_common.hpp>
#include <stratosphere/tipc/tipc_service_object_base.hpp>

namespace ams::tipc {

    template<typename T>
    concept IsServiceObjectAllocator = requires (T &t) {
        { t.Allocate() } -> std::convertible_to<ServiceObjectBase *>;
    };

    template<typename T, size_t N> requires IsServiceObject<T>
    class SingletonAllocator final {
        static_assert(N >= 1);
        private:
            T m_singleton;
        public:
            constexpr ALWAYS_INLINE SingletonAllocator() : m_singleton() { /* ... */ }

            ALWAYS_INLINE ServiceObjectBase *Allocate() { return std::addressof(m_singleton); }
    };

    template<typename T, size_t N> requires IsServiceObject<T>
    class SlabAllocator final : public ServiceObjectDeleter {
        private:
            struct Entry {
                bool used;
                util::TypedStorage<T> storage;
            };
        private:
            Entry m_entries[N];
            os::SdkMutex m_mutex;
        public:
            constexpr ALWAYS_INLINE SlabAllocator() : m_entries(), m_mutex() { /* ... */ }

            T *Allocate() {
                std::scoped_lock lk(m_mutex);

                for (size_t i = 0; i < N; ++i) {
                    if (!m_entries[i].used) {
                        m_entries[i].used = true;
                        return util::ConstructAt(m_entries[i].storage);
                    }
                }

                return nullptr;
            }

            void Deallocate(ServiceObjectBase *object) {
                std::scoped_lock lk(m_mutex);

                for (size_t i = 0; i < N; ++i) {
                    if (m_entries[i].used && GetPointer(m_entries[i].storage) == object) {
                        util::DestroyAt(m_entries[i].storage);
                        m_entries[i].used = false;
                        return;
                    }
                }

                AMS_ABORT("Failed to deallocate entry in SlabAllocator<T, N>");
            }
        public:
            virtual void DeleteServiceObject(ServiceObjectBase *object) override {
                return this->Deallocate(object);
            }
    };
    static_assert(IsServiceObjectAllocator<SlabAllocator<ServiceObjectBase, 1>>);
    static_assert(IsServiceObjectDeleter<SlabAllocator<ServiceObjectBase, 1>>);

}

