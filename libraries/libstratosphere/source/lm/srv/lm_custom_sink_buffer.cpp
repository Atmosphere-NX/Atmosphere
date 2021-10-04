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
#include <stratosphere.hpp>
#include "lm_custom_sink_buffer.hpp"

namespace ams::lm::srv {

    bool CustomSinkBuffer::TryPush(const void *data, size_t size) {
        /* Check pre-conditions. */
        AMS_ASSERT(size <= m_buffer_size);
        AMS_ASSERT(data || size == 0);

        /* If we have nothing to push, succeed. */
        if (size == 0) {
            return true;
        }

        /* Check that we can push the data. */
        if (size > m_buffer_size - m_used_buffer_size) {
            return false;
        }

        /* Push the data. */
        std::memcpy(m_buffer + m_used_buffer_size, data, size);
        m_used_buffer_size += size;

        return true;
    }

    bool CustomSinkBuffer::TryFlush() {
        /* Check that we have data to flush. */
        if (m_used_buffer_size == 0) {
            return false;
        }

        /* Try to flush the data. */
        if (!m_flush_function(m_buffer, m_used_buffer_size)) {
            return false;
        }

        /* Clear our used size. */
        m_used_buffer_size = 0;

        return true;
    }

}
