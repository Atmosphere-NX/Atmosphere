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

    class KAutoObjectWithListContainer {
        NON_COPYABLE(KAutoObjectWithListContainer);
        NON_MOVEABLE(KAutoObjectWithListContainer);
        public:
            using ListType = util::IntrusiveRedBlackTreeMemberTraits<&KAutoObjectWithList::m_list_node>::TreeType<KAutoObjectWithList>;
        public:
            class ListAccessor : public KScopedLightLock {
                private:
                    ListType &m_list;
                public:
                    explicit ListAccessor(KAutoObjectWithListContainer *container) : KScopedLightLock(container->m_lock), m_list(container->m_object_list) { /* ... */ }
                    explicit ListAccessor(KAutoObjectWithListContainer &container) : KScopedLightLock(container.m_lock), m_list(container.m_object_list) { /* ... */ }

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

            friend class ListAccessor;
        private:
            KLightLock m_lock;
            ListType m_object_list;
        public:
            constexpr KAutoObjectWithListContainer() : m_lock(), m_object_list() { MESOSPHERE_ASSERT_THIS(); }

            void Initialize() { MESOSPHERE_ASSERT_THIS(); }
            void Finalize() { MESOSPHERE_ASSERT_THIS(); }

            void Register(KAutoObjectWithList *obj) {
                MESOSPHERE_ASSERT_THIS();

                KScopedLightLock lk(m_lock);

                m_object_list.insert(*obj);
            }

            void Unregister(KAutoObjectWithList *obj) {
                MESOSPHERE_ASSERT_THIS();

                KScopedLightLock lk(m_lock);

                m_object_list.erase(m_object_list.iterator_to(*obj));
            }

            template<typename T> requires (std::derived_from<T, KAutoObjectWithList> && requires (const T &t) { { t.GetOwner() } -> std::convertible_to<const KProcess *>; })
            size_t GetOwnedCount(const KProcess *owner) {
                MESOSPHERE_ASSERT_THIS();

                KScopedLightLock lk(m_lock);

                size_t count = 0;

                for (const auto &obj : m_object_list) {
                    if (const T * const derived = obj.DynamicCast<T *>(); derived != nullptr && derived->GetOwner() == owner) {
                        ++count;
                    }
                }

                return count;
            }
    };


}
