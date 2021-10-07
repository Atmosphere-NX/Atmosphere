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

    ssize_t DNSSerializer::CheckToBufferArguments(const u8 *dst, size_t dst_size, size_t required, int error_id) {
        /* TODO: Logging, using error_id */
        AMS_UNUSED(error_id);

        if (dst == nullptr) {
            return -1;
        } else if (dst_size < required) {
            return -1;
        }

        return 0;
    }

    u32 DNSSerializer::InternalHton(const u32 &v) {
        return ams::socket::InetHtonl(v);
    }

    u16 DNSSerializer::InternalHton(const u16 &v) {
        return ams::socket::InetHtons(v);
    }

    u32 DNSSerializer::InternalNtoh(const u32 &v) {
        return ams::socket::InetNtohl(v);
    }

    u16 DNSSerializer::InternalNtoh(const u16 &v) {
        return ams::socket::InetNtohs(v);
    }


}
