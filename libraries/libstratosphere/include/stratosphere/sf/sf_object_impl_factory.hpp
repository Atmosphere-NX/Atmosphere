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
#include <stratosphere/sf/sf_common.hpp>
#include <stratosphere/sf/sf_allocation_policies.hpp>
#include <stratosphere/sf/impl/sf_service_object_impl.hpp>

namespace ams::sf {

    namespace impl {

        struct StatelessDummyAllocator{};

        template<typename Base, typename Policy>
        class ObjectImplFactoryWithStatelessAllocator {
            public:
                class Object;
                using Allocator = StatelessDummyAllocator;
                using StatelessAllocator = typename Policy::template StatelessAllocator<Object>;

                class Object final : private ::ams::sf::impl::ServiceObjectImplBase2, public Base {
                    NON_COPYABLE(Object);
                    NON_MOVEABLE(Object);
                    friend class ObjectImplFactoryWithStatelessAllocator;
                    private:
                        template<typename... Args>
                        explicit Object(Args &&... args) : Base(std::forward<Args>(args)...) { /* ... */ }

                        static void *operator new(size_t size) {
                            return Policy::template AllocateAligned<Object>(size, alignof(Object));
                        }

                        static void operator delete(void *ptr, size_t size) {
                            return Policy::template DeallocateAligned<Object>(ptr, size, alignof(Object));
                        }

                        static void *operator new(size_t size, Allocator *);
                        static void operator delete(void *ptr, Allocator *);

                        void DisposeImpl() {
                            delete this;
                        }
                    public:
                        void AddReference() {
                            ServiceObjectImplBase2::AddReferenceImpl();
                        }

                        void Release() {
                            if (ServiceObjectImplBase2::ReleaseImpl()) {
                                this->DisposeImpl();
                            }
                        }

                        Allocator *GetAllocator() const {
                            return nullptr;
                        }
                };

                template<typename... Args>
                static Object *Create(Args &&... args) {
                    return new Object(std::forward<Args>(args)...);
                }

                template<typename... Args>
                static Object *Create(Allocator *, Args &&... args) {
                    return new Object(std::forward<Args>(args)...);
                }
        };

        template<typename Base, typename Policy>
        class ObjectImplFactoryWithStatefulAllocator {
            public:
                using Allocator = typename Policy::Allocator;

                class Object final : private ::ams::sf::impl::ServiceObjectImplBase2, public Base {
                    NON_COPYABLE(Object);
                    NON_MOVEABLE(Object);
                    friend class ObjectImplFactoryWithStatefulAllocator;
                    private:
                        Allocator *m_allocator;
                    private:
                        template<typename... Args>
                        explicit Object(Args &&... args) : Base(std::forward<Args>(args)...) { /* ... */ }

                        static void *operator new(size_t size);

                        static void operator delete(void *ptr, size_t size) {
                            AMS_UNUSED(ptr, size);
                        }

                        static void *operator new(size_t size, Allocator *a) {
                            return Policy::AllocateAligned(a, size, alignof(Object));
                        }

                        static void operator delete(void *ptr, Allocator *a) {
                            return Policy::DeallocateAligned(a, ptr, sizeof(Object), alignof(Object));
                        }

                        void DisposeImpl() {
                            Allocator *a = this->GetAllocator();
                            std::destroy_at(this);
                            operator delete(this, a);
                        }
                    public:
                        void AddReference() {
                            ServiceObjectImplBase2::AddReferenceImpl();
                        }

                        void Release() {
                            if (ServiceObjectImplBase2::ReleaseImpl()) {
                                this->DisposeImpl();
                            }
                        }

                        Allocator *GetAllocator() const {
                            return m_allocator;
                        }
                };

                template<typename... Args>
                static Object *Create(Allocator *a, Args &&... args) {
                    auto *ptr = new (a) Object(std::forward<Args>(args)...);
                    if (ptr != nullptr) {
                        ptr->m_allocator = a;
                    }
                    return ptr;
                }
        };

    }

    template<typename Base, typename Policy>
    class ObjectImplFactory;

    template<typename Base, typename Policy> requires (!IsStatefulPolicy<Policy>)
    class ObjectImplFactory<Base, Policy> : public impl::ObjectImplFactoryWithStatelessAllocator<Base, Policy>{};

    template<typename Base, typename Policy> requires (IsStatefulPolicy<Policy>)
    class ObjectImplFactory<Base, Policy> : public impl::ObjectImplFactoryWithStatefulAllocator<Base, Policy>{};

}
