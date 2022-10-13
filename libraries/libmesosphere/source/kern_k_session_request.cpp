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

    Result KSessionRequest::SessionMappings::PushMap(KProcessAddress client, KProcessAddress server, size_t size, KMemoryState state, size_t index) {
        /* At most 15 buffers of each type (4-bit descriptor counts). */
        MESOSPHERE_ASSERT(index < NumMappings);

        /* Get the mapping. */
        Mapping *mapping;
        if (index < NumStaticMappings) {
            mapping = std::addressof(m_static_mappings[index]);
        } else {
            /* Allocate dynamic mappings as necessary. */
            if (m_dynamic_mappings == nullptr) {
                m_dynamic_mappings = DynamicMappings::Allocate();
                R_UNLESS(m_dynamic_mappings != nullptr, svc::ResultOutOfMemory());
            }

            mapping = std::addressof(m_dynamic_mappings->Get(index - NumStaticMappings));
        }

        /* Set the mapping. */
        mapping->Set(client, server, size, state);

        R_SUCCEED();
    }

    Result KSessionRequest::SessionMappings::PushSend(KProcessAddress client, KProcessAddress server, size_t size, KMemoryState state) {
        MESOSPHERE_ASSERT(m_num_recv == 0);
        MESOSPHERE_ASSERT(m_num_exch == 0);
        R_RETURN(this->PushMap(client, server, size, state, m_num_send++));
    }

    Result KSessionRequest::SessionMappings::PushReceive(KProcessAddress client, KProcessAddress server, size_t size, KMemoryState state) {
        MESOSPHERE_ASSERT(m_num_exch == 0);
        R_RETURN(this->PushMap(client, server, size, state, m_num_send + m_num_recv++));
    }

    Result KSessionRequest::SessionMappings::PushExchange(KProcessAddress client, KProcessAddress server, size_t size, KMemoryState state) {
        R_RETURN(this->PushMap(client, server, size, state, m_num_send + m_num_recv + m_num_exch++));
    }

    void KSessionRequest::SessionMappings::Finalize() {
        if (m_dynamic_mappings) {
            DynamicMappings::Free(m_dynamic_mappings);
            m_dynamic_mappings = nullptr;
        }
    }

}
