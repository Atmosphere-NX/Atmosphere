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
#include "../socket_allocator.hpp"
#include "serializer.hpp"

namespace ams::mitm::socket::resolver::serializer {

    namespace {

        template<typename T>
        concept IsSockAddrIn = std::same_as<T, ams::socket::SockAddrIn> || std::same_as<T, struct sockaddr_in>;

        template<typename T> requires IsSockAddrIn<T>
        size_t SizeOfImpl(const T &in) {
            size_t rc = 0;
            rc += sizeof(u16);
            rc += sizeof(u16);
            rc += DNSSerializer::SizeOf(in.sin_addr);
            rc += sizeof(in.sin_zero);
            return rc;
        }

        template<typename T> requires IsSockAddrIn<T>
        ssize_t ToBufferImpl(u8 * const dst, size_t dst_size, const T &in) {
            ssize_t rc = -1;
            u8 *cur = dst;

            if ((rc = DNSSerializer::CheckToBufferArguments(cur, dst_size, sizeof(in), __LINE__)) == -1) {
                return rc;
            }

            const u16 sin_family = static_cast<u16>(in.sin_family);
            if ((rc = DNSSerializer::ToBuffer(cur, dst_size - (cur - dst), sin_family)) == -1) {
                return rc;
            }
            cur += rc;

            const u16 sin_port = static_cast<u16>(in.sin_port);
            if ((rc = DNSSerializer::ToBuffer(cur, dst_size - (cur - dst), sin_port)) == -1) {
                return rc;
            }
            cur += rc;

            const u32 s_addr = static_cast<u32>(in.sin_addr.s_addr);
            if ((rc = DNSSerializer::ToBuffer(cur, dst_size - (cur - dst), s_addr)) == -1) {
                return rc;
            }
            cur += rc;

            if (dst_size - (cur - dst) < sizeof(in.sin_zero)) {
                rc = -1;
                return rc;
            }

            std::memcpy(cur, in.sin_zero, sizeof(in.sin_zero));
            cur += sizeof(in.sin_zero);

            rc = cur - dst;
            return rc;
        }

        template<typename T> requires IsSockAddrIn<T>
        ssize_t FromBufferImpl(T &out, const u8 *src, size_t src_size) {
            ssize_t rc = -1;
            const u8 *cur = src;

            u16 sin_family;
            if ((rc = DNSSerializer::FromBuffer(sin_family, cur, src_size - (cur - src))) == -1) {
                return rc;
            }
            out.sin_family = static_cast<decltype(out.sin_family)>(sin_family);
            cur += rc;

            u16 sin_port;
            if ((rc = DNSSerializer::FromBuffer(sin_port, cur, src_size - (cur - src))) == -1) {
                return rc;
            }
            out.sin_port = static_cast<decltype(out.sin_port)>(sin_port);
            cur += rc;

            u32 s_addr;
            if ((rc = DNSSerializer::FromBuffer(s_addr, cur, src_size - (cur - src))) == -1) {
                return rc;
            }
            out.sin_addr.s_addr = static_cast<decltype(out.sin_addr.s_addr)>(s_addr);
            cur += rc;

            if (src_size - (cur - src) < sizeof(out.sin_zero)) {
                rc = -1;
                return rc;
            }

            std::memcpy(out.sin_zero, cur, sizeof(out.sin_zero));
            cur += sizeof(out.sin_zero);

            rc = cur - src;
            return rc;
        }

    }

    template<> size_t DNSSerializer::SizeOf(const struct sockaddr_in &in) {
        return SizeOfImpl(in);
    }

    template<> size_t DNSSerializer::SizeOf(const ams::socket::SockAddrIn &in) {
        return SizeOfImpl(in);
    }

    template<> ssize_t DNSSerializer::ToBuffer(u8 * const dst, size_t dst_size, const struct sockaddr_in &in) {
        return ToBufferImpl(dst, dst_size, in);
    }

    template<> ssize_t DNSSerializer::ToBuffer(u8 * const dst, size_t dst_size, const ams::socket::SockAddrIn &in) {
        return ToBufferImpl(dst, dst_size, in);
    }

    template<> ssize_t DNSSerializer::FromBuffer(struct sockaddr_in &out, const u8 *src, size_t src_size) {
        return FromBufferImpl(out, src, src_size);
    }

    template<> ssize_t DNSSerializer::FromBuffer(struct ams::socket::SockAddrIn &out, const u8 *src, size_t src_size) {
        return FromBufferImpl(out, src, src_size);
    }

}
