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

    Result PtpObjectDatabase::AddObjectId(const char *parent_name, const char *name, u32 *out_object_id, u32 parent_id, u32 desired_object_id) {
        /* Calculate length of the name. */
        const size_t parent_name_len = std::strlen(parent_name);
        const size_t name_len = std::strlen(name) + 1;
        const size_t alloc_len = parent_name_len + 1 + name_len;

        /* Allocate memory for the node. */
        ObjectNode * const node = m_object_heap->Allocate<ObjectNode>(sizeof(ObjectNode) + alloc_len);
        R_UNLESS(node != nullptr, haze::ResultOutOfMemory());

        {
            /* Ensure we maintain a clean state on failure. */
            auto guard = SCOPE_GUARD { m_object_heap->Deallocate(node, sizeof(ObjectNode) + alloc_len); };

            /* Take ownership of the name. */
            std::strncpy(node->m_name, parent_name, parent_name_len + 1);
            node->m_name[parent_name_len] = '/';
            std::strncpy(node->m_name + parent_name_len + 1, name, name_len);

            /* Check if an object with this name already exists. If it does, we can just return it here. */
            auto it = m_name_to_object_id.find_key(node->m_name);
            if (it != m_name_to_object_id.end()) {
                if (out_object_id) {
                    *out_object_id = it->GetObjectId();
                }
                R_SUCCEED();
            }

            /* Persist the reference to the node. */
            guard.Cancel();
        }

        /* Insert node into trees. */
        node->m_parent_id = parent_id;
        node->m_object_id = desired_object_id == 0 ? m_next_object_id++ : desired_object_id;
        m_name_to_object_id.insert(*node);
        m_object_id_to_name.insert(*node);

        /* Set output. */
        if (out_object_id) {
            *out_object_id = node->GetObjectId();
        }

        /* We succeeded. */
        R_SUCCEED();
    }

    void PtpObjectDatabase::RemoveObjectId(u32 object_id) {
        /* Find in forward mapping. */
        auto it = m_object_id_to_name.find_key(object_id);
        if (it == m_object_id_to_name.end()) {
            return;
        }

        /* Free the node. */
        ObjectNode *node = std::addressof(*it);
        m_object_id_to_name.erase(m_object_id_to_name.iterator_to(*node));
        m_name_to_object_id.erase(m_name_to_object_id.iterator_to(*node));
        m_object_heap->Deallocate(node, sizeof(ObjectNode) + std::strlen(node->GetName()) + 1);
    }

    PtpObjectDatabase::ObjectNode *PtpObjectDatabase::GetObject(u32 object_id) {
        /* Find in forward mapping. */
        auto it = m_object_id_to_name.find_key(object_id);
        if (it == m_object_id_to_name.end()) {
            return nullptr;
        }

        return std::addressof(*it);
    }

}
