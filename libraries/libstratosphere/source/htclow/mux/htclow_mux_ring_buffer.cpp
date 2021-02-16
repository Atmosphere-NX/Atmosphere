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
#include <stratosphere.hpp>
#include "htclow_mux_ring_buffer.hpp"

namespace ams::htclow::mux {

    Result RingBuffer::Write(const void *data, size_t size) {
        /* Validate pre-conditions. */
        AMS_ASSERT(!m_is_read_only);

        /* Check that our buffer can hold the data. */
        R_UNLESS(m_buffer != nullptr,                 htclow::ResultChannelBufferOverflow());
        R_UNLESS(m_data_size + size <= m_buffer_size, htclow::ResultChannelBufferOverflow());

        /* Determine position and copy sizes. */
        const size_t  pos = (m_data_size + m_offset) % m_buffer_size;
        const size_t left = m_buffer_size - pos;
        const size_t over = size - left;

        /* Copy. */
        if (left != 0) {
            std::memcpy(static_cast<u8 *>(m_buffer) + pos, data, left);
        }
        if (over != 0) {
            std::memcpy(m_buffer, static_cast<const u8 *>(data) + left, over);
        }

        /* Update our data size. */
        m_data_size += size;

        return ResultSuccess();
    }

    Result RingBuffer::Copy(void *dst, size_t size) {
        /* Select buffer to discard from. */
        void *buffer = m_is_read_only ? m_read_only_buffer : m_buffer;
        R_UNLESS(buffer != nullptr, htclow::ResultChannelBufferHasNotEnoughData());

        /* Verify that we have enough data. */
        R_UNLESS(m_data_size >= size, htclow::ResultChannelBufferHasNotEnoughData());

        /* Determine position and copy sizes. */
        const size_t pos  = m_offset;
        const size_t left = std::min(m_buffer_size - pos, size);
        const size_t over = size - left;

        /* Copy. */
        if (left != 0) {
            std::memcpy(dst, static_cast<const u8 *>(buffer) + pos, left);
        }
        if (over != 0) {
            std::memcpy(static_cast<u8 *>(dst) + left, buffer, over);
        }

        /* Mark that we can discard. */
        m_can_discard = true;

        return ResultSuccess();
    }

    Result RingBuffer::Discard(size_t size) {
        /* Select buffer to discard from. */
        void *buffer = m_is_read_only ? m_read_only_buffer : m_buffer;
        R_UNLESS(buffer != nullptr, htclow::ResultChannelBufferHasNotEnoughData());

        /* Verify that the data we're discarding has been read. */
        R_UNLESS(m_can_discard, htclow::ResultChannelCannotDiscard());

        /* Verify that we have enough data. */
        R_UNLESS(m_data_size >= size, htclow::ResultChannelBufferHasNotEnoughData());

        /* Discard. */
        m_offset       = (m_offset + size) % m_buffer_size;
        m_data_size   -= size;
        m_can_discard  = false;

        return ResultSuccess();
    }

}
