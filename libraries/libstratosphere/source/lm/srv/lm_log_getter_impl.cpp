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
#include "lm_log_getter_impl.hpp"

namespace ams::lm::srv {

    CustomSinkBuffer &LogGetterImpl::GetBuffer() {
        AMS_FUNCTION_LOCAL_STATIC_CONSTINIT(u8, s_buffer[32_KB]);
        AMS_FUNCTION_LOCAL_STATIC_CONSTINIT(CustomSinkBuffer, s_custom_sink_buffer, s_buffer, sizeof(s_buffer), FlushFunction);

        return s_custom_sink_buffer;
    }

    s64 LogGetterImpl::GetLog(void *buffer, size_t buffer_size, u32 *out_drop_count) {
        /* Check pre-condition. */
        AMS_ASSERT(buffer != nullptr);

        /* Determine how much we can get. */
        size_t min_size = s_buffer_size;
        if (buffer_size < s_buffer_size) {
            min_size = buffer_size;
            IncreaseLogPacketDropCount();
        }

        /* Get the data. */
        std::memcpy(buffer, s_message, min_size);

        /* Set output drop count. */
        *out_drop_count = s_log_packet_drop_count;
        s_log_packet_drop_count = 0;

        return min_size;
    }

}
