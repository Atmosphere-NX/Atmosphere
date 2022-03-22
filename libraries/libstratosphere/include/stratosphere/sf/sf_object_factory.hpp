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
#include <stratosphere/sf/impl/sf_impl_template_base.hpp>
#include <stratosphere/sf/sf_object_impl_factory.hpp>
#include <stratosphere/sf/sf_memory_resource.hpp>
#include <stratosphere/sf/sf_default_allocation_policy.hpp>

namespace ams::sf {

    namespace impl {

        template<typename>
        struct IsSmartPointer : public std::false_type{};

        template<typename T>
        struct IsSmartPointer<SharedPointer<T>> : public std::true_type{};

        template<typename T>
        struct IsSmartPointer<std::shared_ptr<T>> : public std::true_type{};

        template<typename T, typename D>
        struct IsSmartPointer<std::unique_ptr<T, D>> : public std::true_type{};

        template<typename Impl>
        struct UnmanagedEmplaceImplHolderBaseGetter {
            using Type = Impl;
        };

        template<typename Impl> requires (std::is_abstract<Impl>::value)
        struct UnmanagedEmplaceImplHolderBaseGetter<Impl> {
            class Impl2 : public Impl {
                public:
                    using Impl::Impl;
                private:
                    constexpr virtual void AddReference() override { /* ... */ }
                    constexpr virtual void Release() override { /* ... */ }
            };

            using Type = Impl2;
        };

        template<typename Impl>
        class UnmanagedEmplacedImplHolder {
            template<typename, typename, typename, typename, typename>
            friend class impl::ImplTemplateBaseT;
            private:
                using Impl2 = typename UnmanagedEmplaceImplHolderBaseGetter<Impl>::Type;
                static_assert(!std::is_abstract<Impl2>::value);
            private:
                Impl2 m_impl;
            private:
                template<typename... Args>
                constexpr explicit UnmanagedEmplacedImplHolder(Args &&... args) : m_impl(std::forward<Args>(args)...) { /* ... */ }
            public:
                static constexpr Impl *GetImplPointer(UnmanagedEmplacedImplHolder *holder) {
                    return std::addressof(holder->m_impl);
                }
        };

        template<typename Impl>
        class EmplacedImplHolder : private Impl {
            template<typename, typename, typename, typename, typename>
            friend class impl::ImplTemplateBaseT;
            private:
                template<typename... Args>
                constexpr explicit EmplacedImplHolder(Args &&... args) : Impl(std::forward<Args>(args)...) { /* ... */ }
            public:
                static constexpr Impl *GetImplPointer(EmplacedImplHolder *holder) {
                    return holder;
                }

                template<typename Interface>
                static constexpr Impl *GetEmplacedImplPointerImpl(const SharedPointer<Interface> &sp) {
                    using Base = impl::ImplTemplateBase<Interface, Interface, EmplacedImplHolder, EmplacedImplHolder>;
                    return static_cast<Base *>(sp.Get());
                }
        };

        template<typename Impl> requires (IsSmartPointer<Impl>::value)
        class EmplacedImplHolder<Impl> : private Impl {
            template<typename, typename, typename, typename, typename>
            friend class impl::ImplTemplateBaseT;
            private:
                template<typename... Args>
                constexpr explicit EmplacedImplHolder(Args &&... args) : Impl(std::forward<Args>(args)...) { /* ... */ }
            public:
                static constexpr auto *GetImplPointer(EmplacedImplHolder *holder) {
                    return static_cast<Impl *>(holder)->operator ->();
                }

                template<typename Interface>
                static constexpr Impl *GetEmplacedImplPointerImpl(const SharedPointer<Interface> &sp) {
                    using Base = impl::ImplTemplateBase<Interface, Interface, EmplacedImplHolder, EmplacedImplHolder>;
                    return static_cast<Base *>(sp.Get());
                }
        };

        template<typename T>
        using SmartPointerHolder = EmplacedImplHolder<T>;

        template<typename T>
        class ManagedPointerHolder {
            template<typename, typename, typename, typename, typename>
            friend class impl::ImplTemplateBaseT;
            private:
                T *m_p;
            private:
                constexpr explicit ManagedPointerHolder(T *p) : m_p(p) { /* ... */ }
                constexpr ~ManagedPointerHolder() { m_p->Release(); }

                static constexpr T *GetImplPointer(ManagedPointerHolder *holder) {
                    return holder->m_p;
                }
        };

        template<typename T>
        class UnmanagedPointerHolder {
            template<typename, typename, typename, typename, typename>
            friend class impl::ImplTemplateBaseT;
            private:
                T *m_p;
            private:
                constexpr explicit UnmanagedPointerHolder(T *p) : m_p(p) { /* ... */ }

                static constexpr T *GetImplPointer(UnmanagedPointerHolder *holder) {
                    return holder->m_p;
                }
        };

    }

    template<typename Interface, typename Impl>
    class UnmanagedServiceObject final : public impl::ImplTemplateBase<Interface, Interface, impl::UnmanagedEmplacedImplHolder<Impl>, impl::UnmanagedEmplacedImplHolder<Impl>> {
        private:
            using ImplBase = impl::ImplTemplateBase<Interface, Interface, impl::UnmanagedEmplacedImplHolder<Impl>, impl::UnmanagedEmplacedImplHolder<Impl>>;
        public:
            using ImplBase::ImplBase;

            constexpr virtual void AddReference() override { /* ... */ }
            constexpr virtual void Release() override { /* ... */ }

            constexpr Impl &GetImpl() { return *impl::UnmanagedEmplacedImplHolder<Impl>::GetImplPointer(this); }

            constexpr SharedPointer<Interface> GetShared() { return SharedPointer<Interface>(this, false); }
    };

    template<typename Interface, typename T>
    class UnmanagedServiceObjectByPointer final : public impl::ImplTemplateBase<Interface, Interface, impl::UnmanagedPointerHolder<T>, impl::UnmanagedPointerHolder<T>> {
        private:
            using ImplBase = impl::ImplTemplateBase<Interface, Interface, impl::UnmanagedPointerHolder<T>, impl::UnmanagedPointerHolder<T>>;
        public:
            constexpr explicit UnmanagedServiceObjectByPointer(T *ptr) : ImplBase(ptr) { /* ... */ }

            constexpr virtual void AddReference() override { /* ... */ }
            constexpr virtual void Release() override { /* ... */ }

            constexpr SharedPointer<Interface> GetShared() { return SharedPointer<Interface>(this, false); }
    };

    template<typename Policy>
    class ObjectFactory;

    template<typename Interface, typename Impl>
    class EmplacedRef : public SharedPointer<Interface> {
        template<typename> friend class ObjectFactory;
        private:
            constexpr explicit EmplacedRef(Interface *ptr, bool incref) : SharedPointer<Interface>(ptr, incref) { /* ... */ }
        public:
            constexpr EmplacedRef() { /* ... */ }

            constexpr Impl &GetImpl() const {
                return *impl::EmplacedImplHolder<Impl>::template GetEmplacedImplPointerImpl<Interface>(*this);
            }
    };

    template<typename Policy> requires (!IsStatefulPolicy<Policy>)
    class ObjectFactory<Policy> {
        private:
            template<typename Interface, typename Holder, typename T>
            static constexpr SharedPointer<Interface> CreateSharedForPointer(T t) {
                using Base = impl::ImplTemplateBase<Interface, Interface, Holder, Holder>;
                return SharedPointer<Interface>(ObjectImplFactory<Base, Policy>::Create(std::forward<T>(t)), false);
            }
        public:
            template<typename Interface, typename Impl, typename... Args>
            static constexpr EmplacedRef<Interface, Impl> CreateSharedEmplaced(Args &&... args) {
                using Base = impl::ImplTemplateBase<Interface, Interface, impl::EmplacedImplHolder<Impl>, impl::EmplacedImplHolder<Impl>>;
                return EmplacedRef<Interface, Impl>(ObjectImplFactory<Base, Policy>::Create(std::forward<Args>(args)...), false);
            }

            template<typename T, typename... Args>
            static constexpr SharedPointer<T> CreateUserSharedObject(Args &&... args) {
                return SharedPointer<T>(ObjectImplFactory<T, Policy>::Create(std::forward<Args>(args)...), false);
            }

            template<typename Impl, typename Interface>
            static constexpr Impl *GetEmplacedImplPointer(const SharedPointer<Interface> &sp) {
                return impl::EmplacedImplHolder<Impl>::template GetEmplacedImplPointerImpl<Interface>(sp);
            }

            template<typename Interface, typename Smart>
            static constexpr SharedPointer<Interface> CreateShared(Smart &&sp) {
                return CreateSharedForPointer<Interface, impl::SmartPointerHolder<typename std::decay<decltype(sp)>::type>>(std::forward<Smart>(sp));
            }

            template<typename Interface, typename T>
            static constexpr SharedPointer<Interface> CreateShared(T *p) {
                return CreateSharedForPointer<Interface, impl::ManagedPointerHolder<T>>(p);
            }

            template<typename Interface, typename T>
            static constexpr SharedPointer<Interface> CreateSharedWithoutManagement(T *p) {
                return CreateSharedForPointer<Interface, impl::UnmanagedPointerHolder<T>>(p);
            }
    };

    template<typename Policy> requires (IsStatefulPolicy<Policy>)
    class ObjectFactory<Policy> {
        public:
            using Allocator = typename Policy::Allocator;
        private:
            template<typename Interface, typename Holder, typename T>
            static constexpr SharedPointer<Interface> CreateSharedForPointer(Allocator *a, T t) {
                using Base = impl::ImplTemplateBase<Interface, Interface, Holder, Holder>;
                return SharedPointer<Interface>(ObjectImplFactory<Base, Policy>::Create(a, std::forward<T>(t)), false);
            }
        public:
            template<typename Interface, typename Impl, typename... Args>
            static constexpr EmplacedRef<Interface, Impl> CreateSharedEmplaced(Allocator *a, Args &&... args) {
                using Base = impl::ImplTemplateBase<Interface, Interface, impl::EmplacedImplHolder<Impl>, impl::EmplacedImplHolder<Impl>>;
                return EmplacedRef<Interface, Impl>(ObjectImplFactory<Base, Policy>::Create(a, std::forward<Args>(args)...), false);
            }

            template<typename T, typename... Args>
            static constexpr SharedPointer<T> CreateUserSharedObject(Allocator *a, Args &&... args) {
                return SharedPointer<T>(ObjectImplFactory<T, Policy>::Create(a, std::forward<Args>(args)...), false);
            }

            template<typename Impl, typename Interface>
            static constexpr Impl *GetEmplacedImplPointer(const SharedPointer<Interface> &sp) {
                return impl::EmplacedImplHolder<Impl>::template GetEmplacedImplPointerImpl<Interface>(sp);
            }

            template<typename Interface, typename Smart>
            static constexpr SharedPointer<Interface> CreateShared(Allocator *a, Smart &&sp) {
                return CreateSharedForPointer<Interface, impl::SmartPointerHolder<typename std::decay<decltype(sp)>::type>>(a, std::forward<Smart>(sp));
            }

            template<typename Interface, typename T>
            static constexpr SharedPointer<Interface> CreateShared(Allocator *a, T *p) {
                return CreateSharedForPointer<Interface, impl::ManagedPointerHolder<T>>(a, p);
            }

            template<typename Interface, typename T>
            static constexpr SharedPointer<Interface> CreateSharedWithoutManagement(Allocator *a, T *p) {
                return CreateSharedForPointer<Interface, impl::UnmanagedPointerHolder<T>>(a, p);
            }
    };

    template<typename Policy>
    class StatefulObjectFactory {
        public:
            using Allocator = typename Policy::Allocator;
        private:
            using StaticObjectFactory = ObjectFactory<Policy>;
        private:
            Allocator *m_allocator;
        public:
            constexpr explicit StatefulObjectFactory(Allocator *a) : m_allocator(a) { /* ... */ }

            template<typename Interface, typename Impl, typename... Args>
            constexpr EmplacedRef<Interface, Impl> CreateSharedEmplaced(Args &&... args) {
                return StaticObjectFactory::template CreateSharedEmplaced<Interface, Impl>(m_allocator, std::forward<Args>(args)...);
            }

            template<typename Impl, typename Interface>
            static constexpr Impl *GetEmplacedImplPointer(const SharedPointer<Interface> &sp) {
                return StaticObjectFactory::template GetEmplacedImplPointer<Impl, Interface>(sp);
            }

            template<typename Interface, typename Smart>
            constexpr SharedPointer<Interface> CreateShared(Allocator *a, Smart &&sp) {
                return StaticObjectFactory::template CreateShared<Interface>(m_allocator, std::forward<Smart>(sp));
            }

            template<typename Interface, typename T>
            constexpr SharedPointer<Interface> CreateShared(Allocator *a, T *p) {
                return StaticObjectFactory::template CreateShared<Interface>(m_allocator, p);
            }

            template<typename Interface, typename T>
            constexpr SharedPointer<Interface> CreateSharedWithoutManagement(Allocator *a, T *p) {
                return StaticObjectFactory::template CreateSharedWithoutManagement<Interface>(m_allocator, p);
            }
    };

    using DefaultObjectFactory        = ObjectFactory<DefaultAllocationPolicy>;
    using MemoryResourceObjectFactory = ObjectFactory<MemoryResourceAllocationPolicy>;

    template<typename Interface, typename Impl, typename... Args>
    inline EmplacedRef<Interface, Impl> CreateSharedObjectEmplaced(Args &&... args) {
        return DefaultObjectFactory::CreateSharedEmplaced<Interface, Impl>(std::forward<Args>(args)...);
    }

    template<typename Interface, typename Impl, typename... Args>
    inline EmplacedRef<Interface, Impl> CreateSharedObjectEmplaced(MemoryResource *mr, Args &&... args) {
        return MemoryResourceObjectFactory::CreateSharedEmplaced<Interface, Impl>(mr, std::forward<Args>(args)...);
    }

    template<typename Interface, typename Smart>
    inline SharedPointer<Interface> CreateSharedObject(Smart &&sp) {
        return DefaultObjectFactory::CreateShared<Interface, Smart>(std::forward<Smart>(sp));
    }

    template<typename Interface, typename Smart>
    inline SharedPointer<Interface> CreateSharedObject(MemoryResource *mr, Smart &&sp) {
        return MemoryResourceObjectFactory::CreateShared<Interface, Smart>(mr, std::forward<Smart>(sp));
    }

    template<typename Interface, typename T>
    inline SharedPointer<Interface> CreateSharedObject(T *ptr) {
        return DefaultObjectFactory::CreateShared<Interface, T>(std::move(ptr));
    }

    template<typename Interface, typename T>
    inline SharedPointer<Interface> CreateSharedObjectWithoutManagement(T *ptr) {
        return DefaultObjectFactory::CreateSharedWithoutManagement<Interface, T>(std::move(ptr));
    }

    template<typename Interface, typename T>
    inline SharedPointer<Interface> CreateSharedObjectWithoutManagement(MemoryResource *mr, T *ptr) {
        return DefaultObjectFactory::CreateSharedWithoutManagement<Interface, T>(mr, std::move(ptr));
    }

}
