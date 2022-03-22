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
#include "../dnsmitm_debug.hpp"
#include "serializer.hpp"

namespace ams::mitm::socket::resolver::serializer {

    template<> ssize_t DNSSerializer::ToBuffer(u8 * const dst, size_t dst_size, const u16 &in) {
            /* Convert the value. */
            u8 *cur     = dst;
            const u16 val = InternalHton(in);

            /* Check arguments. */
            ssize_t rc = -1;
            if ((rc = CheckToBufferArguments(cur, dst_size, sizeof(u16), __LINE__)) == -1) {
                return rc;
            }

            std::memcpy(cur, std::addressof(val), sizeof(u16));
            rc += sizeof(u16);

            return rc;
    }

    template<> ssize_t DNSSerializer::FromBuffer(u16 &out, const u8 *src, size_t src_size) {
            if (src_size < sizeof(u16)) {
                return -1;
            }

            out = InternalNtoh(*reinterpret_cast<const u16 *>(src));
            return sizeof(u16);
    }

    template<> ssize_t DNSSerializer::ToBuffer(u8 * const dst, size_t dst_size, const u32 &in) {
            /* Convert the value. */
            u8 *cur     = dst;
            const u32 val = InternalHton(in);

            /* Check arguments. */
            ssize_t rc = -1;
            if ((rc = CheckToBufferArguments(cur, dst_size, sizeof(u32), __LINE__)) == -1) {
                return rc;
            }

            std::memcpy(cur, std::addressof(val), sizeof(u32));
            rc += sizeof(u32);

            return rc;
    }

    template<> ssize_t DNSSerializer::FromBuffer(u32 &out, const u8 *src, size_t src_size) {
            if (src_size < sizeof(u32)) {
                return -1;
            }

            out = InternalNtoh(*reinterpret_cast<const u32 *>(src));
            return sizeof(u32);
    }

}
