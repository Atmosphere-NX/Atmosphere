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
#include <mesosphere.hpp>

namespace ams::kern {


    void KAutoObjectWithListContainer::Register(KAutoObjectWithList *obj) {
        MESOSPHERE_ASSERT_THIS();

        KScopedLightLock lk(m_lock);

        m_object_list.insert(*obj);
    }

    void KAutoObjectWithListContainer::Unregister(KAutoObjectWithList *obj) {
        MESOSPHERE_ASSERT_THIS();

        KScopedLightLock lk(m_lock);

        m_object_list.erase(m_object_list.iterator_to(*obj));
    }

    size_t KAutoObjectWithListContainer::GetOwnedCount(KProcess *owner) {
        MESOSPHERE_ASSERT_THIS();

        KScopedLightLock lk(m_lock);

        size_t count = 0;

        for (auto &obj : m_object_list) {
            if (obj.GetOwner() == owner) {
                count++;
            }
        }

        return count;
    }

}
