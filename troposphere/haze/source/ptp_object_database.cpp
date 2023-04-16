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
#include <haze.hpp>

namespace haze {

    void PtpObjectDatabase::Initialize(PtpObjectHeap *object_heap) {
        m_object_heap = object_heap;
        m_object_heap->Initialize();

        std::construct_at(std::addressof(m_name_to_object_id));
        std::construct_at(std::addressof(m_object_id_to_name));

        m_next_object_id = 1;
    }

    void PtpObjectDatabase::Finalize() {
        std::destroy_at(std::addressof(m_object_id_to_name));
        std::destroy_at(std::addressof(m_name_to_object_id));

        m_next_object_id = 0;

        m_object_heap->Finalize();
        m_object_heap = nullptr;
    }

    Result PtpObjectDatabase::CreateOrFindObject(const char *parent_name, const char *name, u32 parent_id, PtpObject **out_object) {
        constexpr auto separator = "/";

        /* Calculate length of the new name with null terminator. */
        const size_t parent_name_len = util::Strlen(parent_name);
        const size_t separator_len   = util::Strlen(separator);
        const size_t name_len        = util::Strlen(name);
        const size_t terminator_len  = 1;
        const size_t alloc_len       = sizeof(PtpObject) + parent_name_len + separator_len + name_len + terminator_len;

        /* Allocate memory for the node. */
        PtpObject * const object = m_object_heap->Allocate<PtpObject>(alloc_len);
        R_UNLESS(object != nullptr, haze::ResultOutOfMemory());

        {
            /* Ensure we maintain a clean state on failure. */
            auto guard = SCOPE_GUARD { m_object_heap->Deallocate(object, alloc_len); };

            /* Take ownership of the name. */
            std::strncpy(object->m_name,                                   parent_name, parent_name_len + terminator_len);
            std::strncpy(object->m_name + parent_name_len,                 separator,   separator_len + terminator_len);
            std::strncpy(object->m_name + parent_name_len + separator_len, name,        name_len + terminator_len);

            /* Check if an object with this name already exists. If it does, we can just return it here. */
            if (auto * const existing = this->GetObjectByName(object->GetName()); existing != nullptr) {
                *out_object = existing;
                R_SUCCEED();
            }

            /* Persist the reference to the node. */
            guard.Cancel();
        }

        /* Set node properties. */
        object->m_parent_id = parent_id;
        object->m_object_id = 0;

        /* Set output. */
        *out_object = object;

        /* We succeeded. */
        R_SUCCEED();
    }

    void PtpObjectDatabase::RegisterObject(PtpObject *object, u32 desired_id) {
        /* If the object is already registered, skip registration. */
        if (object->GetIsRegistered()) {
            return;
        }

        /* Set desired object ID. */
        if (desired_id == 0) {
            desired_id = m_next_object_id++;
        }

        /* Insert node into trees. */
        object->m_object_id = desired_id;
        m_name_to_object_id.insert(*object);
        m_object_id_to_name.insert(*object);
    }

    void PtpObjectDatabase::UnregisterObject(PtpObject *object) {
        /* If the object is not registered, skip trying to unregister. */
        if (!object->GetIsRegistered()) {
            return;
        }

        /* Remove node from trees. */
        m_object_id_to_name.erase(m_object_id_to_name.iterator_to(*object));
        m_name_to_object_id.erase(m_name_to_object_id.iterator_to(*object));
        object->m_object_id = 0;
    }

    void PtpObjectDatabase::DeleteObject(PtpObject *object) {
        /* Unregister the object as required. */
        this->UnregisterObject(object);

        /* Free the object. */
        m_object_heap->Deallocate(object, sizeof(PtpObject) + std::strlen(object->GetName()) + 1);
    }

    Result PtpObjectDatabase::CreateAndRegisterObjectId(const char *parent_name, const char *name, u32 parent_id, u32 *out_object_id) {
        /* Try to create the object. */
        PtpObject *object;
        R_TRY(this->CreateOrFindObject(parent_name, name, parent_id, std::addressof(object)));

        /* We succeeded, so register it. */
        this->RegisterObject(object);

        /* Set the output ID. */
        *out_object_id = object->GetObjectId();

        R_SUCCEED();
    }

    PtpObject *PtpObjectDatabase::GetObjectById(u32 object_id) {
        /* Find in ID mapping. */
        auto it = m_object_id_to_name.find_key(object_id);
        if (it == m_object_id_to_name.end()) {
            return nullptr;
        }

        return std::addressof(*it);
    }

    PtpObject *PtpObjectDatabase::GetObjectByName(const char *name) {
        /* Find in name mapping. */
        auto it = m_name_to_object_id.find_key(name);
        if (it == m_name_to_object_id.end()) {
            return nullptr;
        }

        return std::addressof(*it);
    }
}
