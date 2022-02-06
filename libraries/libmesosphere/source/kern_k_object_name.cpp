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
#include <mesosphere.hpp>

namespace ams::kern {

    namespace {

        constinit KLightLock g_object_list_lock;
        constinit KObjectName::List g_object_list;

    }

    void KObjectName::Initialize(KAutoObject *obj, const char *name) {
        /* Set member variables. */
        m_object = obj;
        std::strncpy(m_name, name, sizeof(m_name));
        m_name[sizeof(m_name) - 1] = '\x00';

        /* Open a reference to the object we hold. */
        m_object->Open();
    }

    bool KObjectName::MatchesName(const char *name) const {
        return std::strncmp(m_name, name, sizeof(m_name)) == 0;
    }

    Result KObjectName::NewFromName(KAutoObject *obj, const char *name) {
        /* Create a new object name. */
        KObjectName *new_name = KObjectName::Allocate();
        R_UNLESS(new_name != nullptr, svc::ResultOutOfResource());

        /* Initialize the new name. */
        new_name->Initialize(obj, name);

        /* Check if there's an existing name. */
        {
            /* Ensure we have exclusive access to the global list. */
            KScopedLightLock lk(g_object_list_lock);

            /* If the object doesn't exist, put it into the list. */
            KScopedAutoObject existing_object = FindImpl(name);
            if (existing_object.IsNull()) {
                g_object_list.push_back(*new_name);
                R_SUCCEED();
            }
        }

        /* The object already exists, which is an error condition. Perform cleanup. */
        obj->Close();
        KObjectName::Free(new_name);
        R_THROW(svc::ResultInvalidState());
    }

    Result KObjectName::Delete(KAutoObject *obj, const char *compare_name) {
        /* Ensure we have exclusive access to the global list. */
        KScopedLightLock lk(g_object_list_lock);

        /* Find a matching entry in the list, and delete it. */
        for (auto &name : g_object_list) {
            if (name.MatchesName(compare_name) && obj == name.GetObject()) {
                /* We found a match, clean up its resources. */
                obj->Close();
                g_object_list.erase(g_object_list.iterator_to(name));
                KObjectName::Free(std::addressof(name));
                R_SUCCEED();
            }
        }

        /* We didn't find the object in the list. */
        R_THROW(svc::ResultNotFound());
    }

    KScopedAutoObject<KAutoObject> KObjectName::Find(const char *name) {
        /* Ensure we have exclusive access to the global list. */
        KScopedLightLock lk(g_object_list_lock);

        return FindImpl(name);
    }

    KScopedAutoObject<KAutoObject> KObjectName::FindImpl(const char *compare_name) {
        /* Try to find a matching object in the global list. */
        for (const auto &name : g_object_list) {
            if (name.MatchesName(compare_name)) {
                return name.GetObject();
            }
        }

        /* There's no matching entry in the list. */
        return nullptr;
    }

}
