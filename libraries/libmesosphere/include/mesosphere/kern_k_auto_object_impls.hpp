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
#include <mesosphere/kern_k_class_token.hpp>

namespace ams::kern {

    /* NOTE: This header is included after all other KAutoObjects. */
    namespace impl {

        template<typename T> requires std::derived_from<T, KAutoObject>
        consteval bool IsAutoObjectInheritanceValidImpl() {
            #define CLASS_TOKEN_HANDLER(CLASSNAME)                                                                               \
                if constexpr (std::same_as<T, CLASSNAME>) {                                                                      \
                    if (T::GetStaticTypeObj().GetClassToken() != ::ams::kern::ClassToken<CLASSNAME>) {                           \
                        return false;                                                                                            \
                    }                                                                                                            \
                } else {                                                                                                         \
                    if (T::GetStaticTypeObj().IsDerivedFrom(CLASSNAME::GetStaticTypeObj()) != std::derived_from<T, CLASSNAME>) { \
                        return false;                                                                                            \
                    }                                                                                                            \
                }

            FOR_EACH_K_CLASS_TOKEN_OBJECT_TYPE(CLASS_TOKEN_HANDLER)
            #undef CLASS_TOKEN_HANDLER

            return true;
        }

        consteval bool IsEveryAutoObjectInheritanceValid() {
            #define CLASS_TOKEN_HANDLER(CLASSNAME) if (!IsAutoObjectInheritanceValidImpl<CLASSNAME>()) { return false; }
            FOR_EACH_K_CLASS_TOKEN_OBJECT_TYPE(CLASS_TOKEN_HANDLER)
            #undef CLASS_TOKEN_HANDLER

            return true;
        }

        static_assert(IsEveryAutoObjectInheritanceValid());

        template<typename T>
        concept IsAutoObjectWithSpecializedGetId = std::derived_from<T, KAutoObjectWithList> && requires (const T &t, const KAutoObjectWithList &l) {
            { t.GetIdImpl() } -> std::same_as<decltype(l.GetId())>;
        };

        template<typename T>
        struct AutoObjectWithListComparatorImpl {
            using RedBlackKeyType = u64;

            static ALWAYS_INLINE RedBlackKeyType GetRedBlackKey(const RedBlackKeyType &v) { return v; }

            static ALWAYS_INLINE RedBlackKeyType GetRedBlackKey(const KAutoObjectWithList &v) {
                if constexpr (IsAutoObjectWithSpecializedGetId<T>) {
                    return static_cast<const T &>(v).GetIdImpl();
                } else {
                    return reinterpret_cast<u64>(std::addressof(v));
                }
            }

            template<typename U> requires (std::same_as<U, KAutoObjectWithList> || std::same_as<U, RedBlackKeyType>)
            static ALWAYS_INLINE int Compare(const U &lhs, const KAutoObjectWithList &rhs) {
                const u64 lid = GetRedBlackKey(lhs);
                const u64 rid = GetRedBlackKey(rhs);

                if (lid < rid) {
                    return -1;
                } else if (lid > rid) {
                    return 1;
                } else {
                    return 0;
                }
            }
        };

        template<typename T>
        using AutoObjectWithListComparator = AutoObjectWithListComparatorImpl<typename std::conditional<IsAutoObjectWithSpecializedGetId<T>, T, KAutoObjectWithList>::type>;

        template<typename T>
        using TrueObjectContainerListType = typename KAutoObjectWithListContainer<T>::ListType<AutoObjectWithListComparator<T>>;

        template<typename T>
        ALWAYS_INLINE TrueObjectContainerListType<T> &GetTrueObjectContainerList(typename KAutoObjectWithListContainer<T>::DummyListType &l) {
            static_assert(alignof(l) == alignof(impl::TrueObjectContainerListType<T>));
            static_assert(sizeof(l)  == sizeof(impl::TrueObjectContainerListType<T>));
            return *reinterpret_cast<TrueObjectContainerListType<T> *>(std::addressof(l));
        }

    }

    ALWAYS_INLINE void KAutoObject::ScheduleDestruction() {
        MESOSPHERE_ASSERT_THIS();

        /* Set our object to destroy. */
        m_next_closed_object = GetCurrentThread().GetClosedObject();

        /* Set ourselves as the thread's next object to destroy. */
        GetCurrentThread().SetClosedObject(this);
    }

    template<typename T>
    class KAutoObjectWithListContainer<T>::ListAccessor : public impl::KAutoObjectWithListContainerBase::ListAccessorImpl<impl::TrueObjectContainerListType<T>> {
        NON_COPYABLE(ListAccessor);
        NON_MOVEABLE(ListAccessor);
        private:
            using BaseListAccessor = impl::KAutoObjectWithListContainerBase::ListAccessorImpl<impl::TrueObjectContainerListType<T>>;
        public:
            explicit ALWAYS_INLINE ListAccessor(KAutoObjectWithListContainer *container) : BaseListAccessor(container, impl::GetTrueObjectContainerList<T>(container->m_dummy_object_list)) { /* ... */ }
            explicit ALWAYS_INLINE ListAccessor(KAutoObjectWithListContainer &container) : BaseListAccessor(container, impl::GetTrueObjectContainerList<T>(container.m_dummy_object_list)) { /* ... */ }

            ALWAYS_INLINE ~ListAccessor() { /* ... */ }
    };


    template<typename T>
    ALWAYS_INLINE void KAutoObjectWithListContainer<T>::Register(T *obj) {
        return this->RegisterImpl(obj, impl::GetTrueObjectContainerList<T>(m_dummy_object_list));
    }

    template<typename T>
    ALWAYS_INLINE void KAutoObjectWithListContainer<T>::Unregister(T *obj) {
        return this->UnregisterImpl(obj, impl::GetTrueObjectContainerList<T>(m_dummy_object_list));
    }

    template<typename T>
    ALWAYS_INLINE size_t KAutoObjectWithListContainer<T>::GetOwnedCountChecked(const KProcess *owner) {
        return this->GetOwnedCountImpl<T>(owner, impl::GetTrueObjectContainerList<T>(m_dummy_object_list));
    }

    inline u64 KAutoObjectWithList::GetId() const {
        #define CLASS_TOKEN_HANDLER(CLASSNAME)                                                                      \
            if constexpr (impl::IsAutoObjectWithSpecializedGetId<CLASSNAME>) {                                      \
                if (const CLASSNAME * const derived = this->DynamicCast<const CLASSNAME *>(); derived != nullptr) { \
                    return []<typename T>(const T * const t_derived) ALWAYS_INLINE_LAMBDA -> u64 {                  \
                        static_assert(std::same_as<T, CLASSNAME>);                                                  \
                        if constexpr (impl::IsAutoObjectWithSpecializedGetId<CLASSNAME>) {                          \
                            return impl::AutoObjectWithListComparator<CLASSNAME>::GetRedBlackKey(*t_derived);       \
                        } else {                                                                                    \
                            AMS_ASSUME(false);                                                                      \
                        }                                                                                           \
                    }(derived);                                                                                     \
                }                                                                                                   \
            }

        FOR_EACH_K_CLASS_TOKEN_OBJECT_TYPE(CLASS_TOKEN_HANDLER)
        #undef CLASS_TOKEN_HANDLER

        return impl::AutoObjectWithListComparator<KAutoObjectWithList>::GetRedBlackKey(*this);
    }

}
