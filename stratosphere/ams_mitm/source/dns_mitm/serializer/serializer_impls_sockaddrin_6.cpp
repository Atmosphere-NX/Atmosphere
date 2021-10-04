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
        concept IsSockAddrIn6 = std::same_as<T, struct sockaddr_in6>;

        template<typename T> requires IsSockAddrIn6<T>
        size_t SizeOfImpl(const T &in) {
            size_t rc = 0;
            rc += sizeof(u16);
            rc += sizeof(u16);
            rc += sizeof(u32);
            rc += DNSSerializer::SizeOf(in.sin6_addr);
            rc += sizeof(u32);
            return rc;
        }

        template<typename T> requires IsSockAddrIn6<T>
        ssize_t ToBufferImpl(u8 * const dst, size_t dst_size, const T &in) {
            ssize_t rc = -1;
            u8 *cur = dst;

            if ((rc = DNSSerializer::CheckToBufferArguments(cur, dst_size, sizeof(in), __LINE__)) == -1) {
                return rc;
            }

            const u16 sin6_family = static_cast<u16>(in.sin6_family);
            if ((rc = DNSSerializer::ToBuffer(cur, dst_size - (cur - dst), sin6_family)) == -1) {
                return rc;
            }
            cur += rc;

            const u16 sin6_port = static_cast<u16>(in.sin6_port);
            if ((rc = DNSSerializer::ToBuffer(cur, dst_size - (cur - dst), sin6_port)) == -1) {
                return rc;
            }
            cur += rc;

            const u32 sin6_flowinfo = static_cast<u32>(in.sin6_flowinfo);
            if ((rc = DNSSerializer::ToBuffer(cur, dst_size - (cur - dst), sin6_flowinfo)) == -1) {
                return rc;
            }
            cur += rc;

            std::memcpy(cur, std::addressof(in.sin6_addr), sizeof(in.sin6_addr));
            cur += sizeof(in.sin6_addr);

            const u32 sin6_scope_id = static_cast<u32>(in.sin6_scope_id);
            if ((rc = DNSSerializer::ToBuffer(cur, dst_size - (cur - dst), sin6_scope_id)) == -1) {
                return rc;
            }
            cur += rc;

            rc = cur - dst;
            return rc;
        }

        template<typename T> requires IsSockAddrIn6<T>
        ssize_t FromBufferImpl(T &out, const u8 *src, size_t src_size) {
            ssize_t rc = -1;
            const u8 *cur = src;

            u16 sin6_family;
            if ((rc = DNSSerializer::FromBuffer(sin6_family, cur, src_size - (cur - src))) == -1) {
                return rc;
            }
            out.sin6_family = static_cast<decltype(out.sin6_family)>(sin6_family);
            cur += rc;

            u16 sin6_port;
            if ((rc = DNSSerializer::FromBuffer(sin6_port, cur, src_size - (cur - src))) == -1) {
                return rc;
            }
            out.sin6_port = static_cast<decltype(out.sin6_port)>(sin6_port);
            cur += rc;

            u32 sin6_flowinfo;
            if ((rc = DNSSerializer::FromBuffer(sin6_flowinfo, cur, src_size - (cur - src))) == -1) {
                return rc;
            }
            out.sin6_flowinfo = static_cast<decltype(out.sin6_flowinfo)>(sin6_flowinfo);
            cur += rc;

            std::memcpy(std::addressof(out.sin6_addr), cur, sizeof(out.sin6_addr));
            cur += sizeof(out.sin6_addr);

            u32 sin6_scope_id;
            if ((rc = DNSSerializer::FromBuffer(sin6_scope_id, cur, src_size - (cur - src))) == -1) {
                return rc;
            }
            out.sin6_scope_id = static_cast<decltype(out.sin6_scope_id)>(sin6_scope_id);
            cur += rc;

            rc = cur - src;
            return rc;
        }

    }

    template<> size_t DNSSerializer::SizeOf(const struct sockaddr_in6 &in) {
        return SizeOfImpl(in);
    }

    template<> ssize_t DNSSerializer::ToBuffer(u8 * const dst, size_t dst_size, const struct sockaddr_in6 &in) {
        return ToBufferImpl(dst, dst_size, in);
    }

    template<> ssize_t DNSSerializer::FromBuffer(struct sockaddr_in6 &out, const u8 *src, size_t src_size) {
        return FromBufferImpl(out, src, src_size);
    }

}
