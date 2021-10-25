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
#include <mesosphere/kern_common.hpp>
#include <mesosphere/kern_k_auto_object.hpp>
#include <mesosphere/kern_k_light_lock.hpp>

namespace ams::kern {

    namespace impl {

        template<typename T>
        struct GetAutoObjectWithListComparator;

        class KAutoObjectWithListContainerBase {
            NON_COPYABLE(KAutoObjectWithListContainerBase);
            NON_MOVEABLE(KAutoObjectWithListContainerBase);
            protected:
                template<typename ListType>
                class ListAccessorImpl {
                    NON_COPYABLE(ListAccessorImpl);
                    NON_MOVEABLE(ListAccessorImpl);
                    private:
                        KScopedLightLock m_lk;
                        ListType &m_list;
                    public:
                        explicit ALWAYS_INLINE ListAccessorImpl(KAutoObjectWithListContainerBase *container, ListType &list) : m_lk(container->m_lock), m_list(list) { /* ... */ }
                        explicit ALWAYS_INLINE ListAccessorImpl(KAutoObjectWithListContainerBase &container, ListType &list) : m_lk(container.m_lock), m_list(list) { /* ... */ }

                        ALWAYS_INLINE ~ListAccessorImpl() { /* ... */ }

                        ALWAYS_INLINE typename ListType::iterator begin() const {
                            return m_list.begin();
                        }

                        ALWAYS_INLINE typename ListType::iterator end() const {
                            return m_list.end();
                        }

                        ALWAYS_INLINE typename ListType::iterator find(typename ListType::const_reference ref) const {
                            return m_list.find(ref);
                        }

                        ALWAYS_INLINE typename ListType::iterator find_key(typename ListType::const_key_reference ref) const {
                            return m_list.find_key(ref);
                        }
                };

                template<typename ListType>
                friend class ListAccessorImpl;
            private:
                KLightLock m_lock;
            protected:
                constexpr KAutoObjectWithListContainerBase() : m_lock() { /* ... */ }

                ALWAYS_INLINE void InitializeImpl() { MESOSPHERE_ASSERT_THIS(); }
                ALWAYS_INLINE void FinalizeImpl() { MESOSPHERE_ASSERT_THIS(); }

                template<typename ListType>
                void RegisterImpl(KAutoObjectWithList *obj, ListType &list) {
                    MESOSPHERE_ASSERT_THIS();

                    KScopedLightLock lk(m_lock);

                    list.insert(*obj);
                }

                template<typename ListType>
                void UnregisterImpl(KAutoObjectWithList *obj, ListType &list) {
                    MESOSPHERE_ASSERT_THIS();

                    KScopedLightLock lk(m_lock);

                    list.erase(list.iterator_to(*obj));
                }

                template<typename U, typename ListType>
                size_t GetOwnedCountImpl(const KProcess *owner, ListType &list) {
                    MESOSPHERE_ASSERT_THIS();

                    KScopedLightLock lk(m_lock);

                    size_t count = 0;

                    for (const auto &obj : list) {
                        MESOSPHERE_AUDIT(obj.DynamicCast<const U *>() != nullptr);
                        if (static_cast<const U &>(obj).GetOwner() == owner) {
                            ++count;
                        }
                    }

                    return count;
                }
        };

        struct DummyKAutoObjectWithListComparator {
            static NOINLINE int Compare(KAutoObjectWithList &lhs, KAutoObjectWithList &rhs) {
                MESOSPHERE_UNUSED(lhs, rhs);
                MESOSPHERE_PANIC("DummyKAutoObjectWithListComparator invoked");
            }
        };

    }

    template<typename T>
    class KAutoObjectWithListContainer : public impl::KAutoObjectWithListContainerBase {
        private:
            using Base = impl::KAutoObjectWithListContainerBase;
        public:
            class ListAccessor;
            friend class ListAccessor;

            template<typename Comparator>
            using ListType = util::IntrusiveRedBlackTreeMemberTraits<&KAutoObjectWithList::m_list_node>::TreeType<Comparator>;

            using DummyListType = ListType<impl::DummyKAutoObjectWithListComparator>;
        private:
            DummyListType m_dummy_object_list;
        public:
            constexpr ALWAYS_INLINE KAutoObjectWithListContainer() : Base(), m_dummy_object_list() { static_assert(std::derived_from<T, KAutoObjectWithList>); }

            ALWAYS_INLINE void Initialize() { return this->InitializeImpl(); }
            ALWAYS_INLINE void Finalize() { return this->FinalizeImpl(); }

            void Register(T *obj);
            void Unregister(T *obj);

        private:
            size_t GetOwnedCountChecked(const KProcess *owner);
        public:
            template<typename U> requires (std::same_as<U, T> && requires (const U &u) { { u.GetOwner() } -> std::convertible_to<const KProcess *>; })
            ALWAYS_INLINE size_t GetOwnedCount(const KProcess *owner) {
                return this->GetOwnedCountChecked(owner);
            }
    };


}
