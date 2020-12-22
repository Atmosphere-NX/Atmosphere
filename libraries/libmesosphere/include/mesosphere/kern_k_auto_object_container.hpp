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
#include <mesosphere/kern_common.hpp>
#include <mesosphere/kern_k_auto_object.hpp>
#include <mesosphere/kern_k_light_lock.hpp>

namespace ams::kern {

    class KAutoObjectWithListContainer {
        NON_COPYABLE(KAutoObjectWithListContainer);
        NON_MOVEABLE(KAutoObjectWithListContainer);
        public:
            using ListType = util::IntrusiveRedBlackTreeMemberTraits<&KAutoObjectWithList::list_node>::TreeType<KAutoObjectWithList>;
        public:
            class ListAccessor : public KScopedLightLock {
                private:
                    ListType &m_list;
                public:
                    explicit ListAccessor(KAutoObjectWithListContainer *container) : KScopedLightLock(container->m_lock), m_list(container->m_object_list) { /* ... */ }
                    explicit ListAccessor(KAutoObjectWithListContainer &container) : KScopedLightLock(container.m_lock), m_list(container.m_object_list) { /* ... */ }

                    typename ListType::iterator begin() const {
                        return m_list.begin();
                    }

                    typename ListType::iterator end() const {
                        return m_list.end();
                    }

                    typename ListType::iterator find(typename ListType::const_reference ref) const {
                        return m_list.find(ref);
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

            void Register(KAutoObjectWithList *obj);
            void Unregister(KAutoObjectWithList *obj);
            size_t GetOwnedCount(KProcess *owner);
    };


}
