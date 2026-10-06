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
        
        /* NOTE: This is local to the nn::socket::resolver::serializer namespace. */
        struct AddrInfoNodeCname {
            struct ams::socket::ares_addrinfo_node *node;
            char *ai_canonname;
        };

        constexpr inline u32 AresMagic = 0xBEEFCAFE;
        
        template<typename T>
        concept IsAresAddrInfo = std::same_as<T, struct ams::socket::ares_addrinfo>;
        
        template<typename T>
        concept IsAresAddrInfoHints = std::same_as<T, struct ams::socket::ares_addrinfo_hints>;
        
        template<typename T>
        concept IsAresAddrInfoNode = std::same_as<T, struct ams::socket::ares_addrinfo_node>;
        
        template<typename T>
        concept IsAddrInfoNodeCname = std::same_as<T, AddrInfoNodeCname>;

        template<typename T> requires IsAresAddrInfoNode<T> || IsAddrInfoNodeCname<T>
        using SockAddrInType = typename std::conditional<std::same_as<T, struct ams::socket::ares_addrinfo> || std::same_as<T, AddrInfoNodeCname>, struct sockaddr_in, struct sockaddr_in>::type;

        template<typename T> requires IsAresAddrInfoNode<T> || IsAddrInfoNodeCname<T>
        using SockAddrIn6Type = typename std::conditional<std::same_as<T, struct ams::socket::ares_addrinfo> || std::same_as<T, AddrInfoNodeCname>, struct sockaddr_in6, struct sockaddr_in6>::type;

        template<typename T> requires IsAresAddrInfoNode<T> || IsAddrInfoNodeCname<T>
        constexpr bool IsAfInet(const auto ai_family) {
            if constexpr (std::same_as<T, struct ams::socket::ares_addrinfo_node> || std::same_as<T, AddrInfoNodeCname>) {
                /* NOTE: Keep this in case a nn::socket::AddrInfo equivalent is ever implemented for c-ares. */
                /* As of 23.0.0 only the ares_addrinfo struct is supported in the serializer. */
                return ai_family == AF_INET;
            } else {
                return ai_family == AF_INET;
            }
        }

        template<typename T> requires IsAresAddrInfoNode<T> || IsAddrInfoNodeCname<T>
        constexpr bool IsAfInet6(const auto ai_family) {
            if constexpr (std::same_as<T, struct ams::socket::ares_addrinfo_node> || std::same_as<T, AddrInfoNodeCname>) {
                /* NOTE: Keep this in case a nn::socket::AddrInfo equivalent is ever implemented for c-ares. */
                /* As of 23.0.0 only the ares_addrinfo struct is supported in the serializer. */
                return ai_family == AF_INET6;
            } else {
                return ai_family == AF_INET6;
            }
        }

        template<typename T> requires IsAresAddrInfoNode<T>
        size_t AddrInfoNodeSizeOf(const T *ares_addr_info_node) {
            size_t rc = 6 * sizeof(u32);

            if (ares_addr_info_node->ai_addr == nullptr) {
                rc += sizeof(u32);
            } else if (IsAfInet<T>(ares_addr_info_node->ai_family)) {
                rc += DNSSerializer::SizeOf(*reinterpret_cast<SockAddrInType<T> *>(ares_addr_info_node->ai_addr));
            } else if (IsAfInet6<T>(ares_addr_info_node->ai_family)) {
                rc += DNSSerializer::SizeOf(*reinterpret_cast<SockAddrIn6Type<T> *>(ares_addr_info_node->ai_addr));
            } else if (ares_addr_info_node->ai_addrlen == 0) {
                rc += sizeof(u32);
            } else {
                rc += ares_addr_info_node->ai_addrlen;
            }

            if (ares_addr_info_node->ai_next == nullptr) {
                rc += sizeof(u32);
            }

            return rc;
        }

        template<typename T> requires IsAresAddrInfo<T>
        size_t SizeOfImpl(const T &in) {
            size_t rc = 0;
            
            const T *ares_addr_info = std::addressof(in);
            for (const struct ams::socket::ares_addrinfo_node *ares_addr_info_node = ares_addr_info->nodes; ares_addr_info_node != nullptr; ares_addr_info_node = ares_addr_info_node->ai_next) {
                rc += AddrInfoNodeSizeOf(ares_addr_info_node);
            }

            return rc;
        }
        
        template<typename T> requires IsAresAddrInfoHints<T>
        size_t SizeOfImpl(const T &) {
            return 5 * sizeof(u32);
        }

        template<typename T> requires IsAddrInfoNodeCname<T>
        ssize_t ToBufferInternalImpl(u8 * const dst, size_t dst_size, const T &addr_info_node_cname) {
            ssize_t rc = -1;
            u8 *cur = dst;

            {
                const u32 value = AresMagic;
                if ((rc = DNSSerializer::ToBuffer(cur, dst_size - (cur - dst), value)) == -1) {
                    return rc;
                }
                cur += rc;
            }

            {
                const u32 value = static_cast<u32>(addr_info_node_cname.node->ai_flags);
                if ((rc = DNSSerializer::ToBuffer(cur, dst_size - (cur - dst), value)) == -1) {
                    return rc;
                }
                cur += rc;
            }

            {
                const u32 value = static_cast<u32>(addr_info_node_cname.node->ai_family);
                if ((rc = DNSSerializer::ToBuffer(cur, dst_size - (cur - dst), value)) == -1) {
                    return rc;
                }
                cur += rc;
            }

            {
                const u32 value = static_cast<u32>(addr_info_node_cname.node->ai_socktype);
                if ((rc = DNSSerializer::ToBuffer(cur, dst_size - (cur - dst), value)) == -1) {
                    return rc;
                }
                cur += rc;
            }

            {
                const u32 value = static_cast<u32>(addr_info_node_cname.node->ai_protocol);
                if ((rc = DNSSerializer::ToBuffer(cur, dst_size - (cur - dst), value)) == -1) {
                    return rc;
                }
                cur += rc;
            }

            {
                const u32 value = static_cast<u32>(addr_info_node_cname.node->ai_addrlen);
                if ((rc = DNSSerializer::ToBuffer(cur, dst_size - (cur - dst), value)) == -1) {
                    return rc;
                }
                cur += rc;
            }

            {
                if (addr_info_node_cname.node->ai_addr == nullptr || addr_info_node_cname.node->ai_addrlen == 0) {
                    const u32 value = 0;
                    if ((rc = DNSSerializer::ToBuffer(cur, dst_size - (cur - dst), value)) == -1) {
                        return rc;
                    }
                } else if (IsAfInet<T>(addr_info_node_cname.node->ai_family)) {
                    if ((rc = DNSSerializer::ToBuffer(cur, dst_size - (cur - dst), *reinterpret_cast<SockAddrInType<T> *>(addr_info_node_cname.node->ai_addr))) == -1) {
                        return rc;
                    }
                } else if (IsAfInet6<T>(addr_info_node_cname.node->ai_family)) {
                    if ((rc = DNSSerializer::ToBuffer(cur, dst_size - (cur - dst), *reinterpret_cast<SockAddrIn6Type<T> *>(addr_info_node_cname.node->ai_addr))) == -1) {
                        return rc;
                    }
                } else {
                    rc = -1;
                    return rc;
                }
                cur += rc;
            }

            {
                if ((rc = DNSSerializer::ToBuffer(cur, dst_size - (cur - dst), addr_info_node_cname.ai_canonname)) == -1) {
                    return rc;
                }
                cur += rc;
            }

            if (addr_info_node_cname.node->ai_next == nullptr) {
                const u32 value = 0;
                if ((rc = DNSSerializer::ToBuffer(cur, dst_size - (cur - dst), value)) == -1) {
                    return rc;
                }
                cur += rc;
            }

            rc = cur - dst;
            return rc;
        }

        template<typename T> requires IsAresAddrInfo<T>
        ssize_t ToBufferImpl(u8 * const dst, size_t dst_size, const T &in) {
            ssize_t rc = -1;
            u8 *cur = dst;
            std::memset(dst, 0, dst_size);

            const size_t required = DNSSerializer::SizeOf(in);
            if ((rc = DNSSerializer::CheckToBufferArguments(cur, dst_size, required, __LINE__)) == -1) {
                return rc;
            }
            
            AddrInfoNodeCname addr_info_node_cname;
            const T *ares_addr_info = std::addressof(in);
            if (ares_addr_info->cnames != nullptr) {
                const struct ams::socket::ares_addrinfo_cname *ares_addr_info_cname;
                for (ares_addr_info_cname = ares_addr_info->cnames; ares_addr_info_cname->next != nullptr; ares_addr_info_cname = ares_addr_info_cname->next) {
                    /* Do nothing. */
                }
                addr_info_node_cname.ai_canonname = ares_addr_info_cname->name;
            } else {
                addr_info_node_cname.ai_canonname = ares_addr_info->name;
            }

            for (struct ams::socket::ares_addrinfo_node *ares_addr_info_node = ares_addr_info->nodes; ares_addr_info_node != nullptr; ares_addr_info_node = ares_addr_info_node->ai_next) {
                addr_info_node_cname.node = ares_addr_info_node;
                if ((rc = ToBufferInternalImpl(cur, dst_size, addr_info_node_cname)) == -1) {
                    return rc;
                }
                
                addr_info_node_cname.ai_canonname = nullptr;
                cur += rc;
            }
            
            rc = cur - dst;
            return rc;
        }
        
        template<typename T> requires IsAresAddrInfoHints<T>
        ssize_t ToBufferImpl(u8 * const dst, size_t dst_size, const T &in) {
            ssize_t rc = -1;
            u8 *cur = dst;
            
            const size_t required = DNSSerializer::SizeOf(in);
            if (dst_size < required) {
                return rc;
            }

            {
                const u32 value = AresMagic;
                if ((rc = DNSSerializer::ToBuffer(cur, dst_size - (cur - dst), value)) == -1) {
                    return rc;
                }
                cur += rc;
            }

            {
                const u32 value = static_cast<u32>(in.ai_flags);
                if ((rc = DNSSerializer::ToBuffer(cur, dst_size - (cur - dst), value)) == -1) {
                    return rc;
                }
                cur += rc;
            }

            {
                const u32 value = static_cast<u32>(in.ai_family);
                if ((rc = DNSSerializer::ToBuffer(cur, dst_size - (cur - dst), value)) == -1) {
                    return rc;
                }
                cur += rc;
            }

            {
                const u32 value = static_cast<u32>(in.ai_socktype);
                if ((rc = DNSSerializer::ToBuffer(cur, dst_size - (cur - dst), value)) == -1) {
                    return rc;
                }
                cur += rc;
            }

            {
                const u32 value = static_cast<u32>(in.ai_protocol);
                if ((rc = DNSSerializer::ToBuffer(cur, dst_size - (cur - dst), value)) == -1) {
                    return rc;
                }
                cur += rc;
            }

            return rc;
        }
        
        template<typename T> requires IsAresAddrInfoHints<T>
        ssize_t FromBufferImpl(T &out, const u8 *src, size_t src_size) {
            ssize_t rc = 0;
            const u8 *cur = src;

            const size_t required = DNSSerializer::SizeOf(out);
            if (src_size < required) {
                ams::socket::SetLastError(ams::socket::Errno::ENoSpc);
                rc = -1;
                return rc;
            }
            
            std::memset(std::addressof(out), 0, sizeof(out));

            u32 tmp_value;

            {
                if ((rc = DNSSerializer::FromBuffer(tmp_value, cur, src_size - (cur - src))) == -1) {
                    return rc;
                } else if (tmp_value != AresMagic) {
                    return rc;
                }
                cur += rc;
            }

            {
                if ((rc = DNSSerializer::FromBuffer(tmp_value, cur, src_size - (cur - src))) == -1) {
                    return rc;
                }
                out.ai_flags = static_cast<decltype(out.ai_flags)>(tmp_value);
                cur += rc;
            }

            {
                if ((rc = DNSSerializer::FromBuffer(tmp_value, cur, src_size - (cur - src))) == -1) {
                    return rc;
                }
                out.ai_family = static_cast<decltype(out.ai_family)>(tmp_value);
                cur += rc;
            }

            {
                if ((rc = DNSSerializer::FromBuffer(tmp_value, cur, src_size - (cur - src))) == -1) {
                    return rc;
                }
                out.ai_socktype = static_cast<decltype(out.ai_socktype)>(tmp_value);
                cur += rc;
            }

            {
                if ((rc = DNSSerializer::FromBuffer(tmp_value, cur, src_size - (cur - src))) == -1) {
                    return rc;
                }
                out.ai_protocol = static_cast<decltype(out.ai_protocol)>(tmp_value);
                cur += rc;
            }

            rc = cur - src;
            return rc;
        }

    }

    template<>  size_t DNSSerializer::SizeOf(const struct ams::socket::ares_addrinfo &in) {
        return SizeOfImpl(in);
    }
    
    template<>  size_t DNSSerializer::SizeOf(const struct ams::socket::ares_addrinfo_hints &in) {
        return SizeOfImpl(in);
    }

    /* NOTE: DNSSerializer::FromBuffer doesn't exist for ares_addrinfo, only DNSSerializer::ToBuffer does. */
    /* Official software pairs it with the old addrinfo DNSSerializer::FromBuffer instead. */
    template<> ssize_t DNSSerializer::ToBuffer(u8 * const dst, size_t dst_size, const struct ams::socket::ares_addrinfo &in) {
        return ToBufferImpl(dst, dst_size, in);
    }
    
    template<> ssize_t DNSSerializer::ToBuffer(u8 * const dst, size_t dst_size, const struct ams::socket::ares_addrinfo_hints &in) {
        return ToBufferImpl(dst, dst_size, in);
    }
    
    template<> ssize_t DNSSerializer::FromBuffer(struct ams::socket::ares_addrinfo_hints &out, const u8 *src, size_t src_size) {
        return FromBufferImpl(out, src, src_size);
    }
}
