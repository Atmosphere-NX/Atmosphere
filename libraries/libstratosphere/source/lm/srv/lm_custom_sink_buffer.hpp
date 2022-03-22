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

namespace ams::lm::srv {

    class CustomSinkBuffer {
        NON_COPYABLE(CustomSinkBuffer);
        NON_MOVEABLE(CustomSinkBuffer);
        public:
            using FlushFunction = bool (*)(const u8 *buffer, size_t buffer_size);
        private:
            u8 *m_buffer;
            size_t m_buffer_size;
            size_t m_used_buffer_size;
            FlushFunction m_flush_function;
        public:
            constexpr explicit CustomSinkBuffer(void *buffer, size_t buffer_size, FlushFunction f) : m_buffer(static_cast<u8 *>(buffer)), m_buffer_size(buffer_size), m_used_buffer_size(0), m_flush_function(f) {
                AMS_ASSERT(m_buffer != nullptr);
                AMS_ASSERT(m_buffer_size > 0);
                AMS_ASSERT(m_flush_function != nullptr);
            }

            bool TryPush(const void *data, size_t size);
            bool TryFlush();
    };

}
