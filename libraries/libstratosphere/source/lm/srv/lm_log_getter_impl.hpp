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
#include <stratosphere.hpp>
#include "lm_custom_sink_buffer.hpp"

namespace ams::lm::srv {

    class LogGetterImpl {
        NON_COPYABLE(LogGetterImpl);
        NON_MOVEABLE(LogGetterImpl);
        private:
            static constinit inline const u8 *s_message = nullptr;
            static constinit inline size_t s_buffer_size = 0;
            static constinit inline size_t s_log_packet_drop_count = 0;
        private:
            LogGetterImpl();
        public:
            static CustomSinkBuffer &GetBuffer();
            static s64 GetLog(void *buffer, size_t buffer_size, u32 *out_drop_count);

            static void IncreaseLogPacketDropCount() { ++s_log_packet_drop_count; }
        private:
            static bool FlushFunction(const u8 *buffer, size_t buffer_size) {
                s_message     = buffer;
                s_buffer_size = buffer_size;
                return true;
            }
    };

}
