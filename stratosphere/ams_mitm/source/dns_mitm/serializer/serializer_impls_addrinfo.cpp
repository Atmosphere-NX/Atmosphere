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

        constexpr inline u32 AddrInfoMagic = 0xBEEFCAFE;

        template<typename T>
        concept IsAddrInfo = std::same_as<T, ams::socket::AddrInfo> || std::same_as<T, struct addrinfo>;

        template<typename T> requires IsAddrInfo<T>
        using SockAddrType   = typename std::conditional<std::same_as<T, ams::socket::AddrInfo>, ams::socket::SockAddr, struct sockaddr>::type;

        template<typename T> requires IsAddrInfo<T>
        using SockAddrInType = typename std::conditional<std::same_as<T, ams::socket::AddrInfo>, ams::socket::SockAddrIn, struct sockaddr_in>::type;

        template<typename T> requires IsAddrInfo<T>
        using SockAddrIn6Type = typename std::conditional<std::same_as<T, ams::socket::AddrInfo>, struct sockaddr_in6, struct sockaddr_in6>::type;

        template<typename T> requires IsAddrInfo<T>
        constexpr bool IsAfInet(const auto ai_family) {
            if constexpr (std::same_as<T, ams::socket::AddrInfo>) {
                return ai_family == ams::socket::Family::Af_Inet;
            } else {
                return ai_family == AF_INET;
            }
        }

        template<typename T> requires IsAddrInfo<T>
        constexpr bool IsAfInet6(const auto ai_family) {
            if constexpr (std::same_as<T, ams::socket::AddrInfo>) {
                return ai_family == ams::socket::Family::Af_Inet6;
            } else {
                return ai_family == AF_INET;
            }
        }

        template<typename T> requires IsAddrInfo<T>
        void FreeAddrInfoImpl(T &addr_info) {
            T *next = nullptr;
            for (T *cur = std::addressof(addr_info); cur != nullptr; cur = next) {
                next = cur->ai_next;

                if (cur->ai_addr != nullptr) {
                    if (IsAfInet<T>(cur->ai_family)) {
                        ams::socket::impl::Free(reinterpret_cast<SockAddrInType<T> *>(cur->ai_addr));
                    } else if (IsAfInet6<T>(cur->ai_family)) {
                        ams::socket::impl::Free(reinterpret_cast<SockAddrIn6Type<T> *>(cur->ai_addr));
                    } else {
                        ams::socket::impl::Free(cur->ai_addr);
                    }
                    cur->ai_addr = nullptr;
                }

                if (cur->ai_canonname != nullptr) {
                    ams::socket::impl::Free(cur->ai_canonname);
                    cur->ai_canonname = nullptr;
                }

                if (cur != std::addressof(addr_info)) {
                    ams::socket::impl::Free(cur);
                }
            }
        }

        template<typename T> requires IsAddrInfo<T>
        size_t AddrInfoSingleSizeOf(const T *addr_info) {
            size_t rc = 6 * sizeof(u32);

            if (addr_info->ai_addr == nullptr) {
                rc += sizeof(u32);
            } else if (IsAfInet<T>(addr_info->ai_family)) {
                rc += DNSSerializer::SizeOf(*reinterpret_cast<SockAddrInType<T> *>(addr_info->ai_addr));
            } else if (IsAfInet6<T>(addr_info->ai_family)) {
                rc += DNSSerializer::SizeOf(*reinterpret_cast<SockAddrIn6Type<T> *>(addr_info->ai_addr));
            } else if (addr_info->ai_addrlen == 0) {
                rc += sizeof(u32);
            } else {
                rc += addr_info->ai_addrlen;
            }

            if (addr_info->ai_canonname != nullptr) {
                rc += DNSSerializer::SizeOf(static_cast<const char *>(addr_info->ai_canonname));
            } else {
                rc += sizeof(u8);
            }

            if (addr_info->ai_next == nullptr) {
                rc += sizeof(u32);
            }

            return rc;
        }

        template<typename T> requires IsAddrInfo<T>
        size_t SizeOfImpl(const T &in) {
            size_t rc = 0;

            for (const T *addr_info = std::addressof(in); addr_info != nullptr; addr_info = addr_info->ai_next) {
                rc += AddrInfoSingleSizeOf(addr_info);
            }

            return rc;
        }

        template<typename T> requires IsAddrInfo<T>
        ssize_t ToBufferInternalImpl(u8 * const dst, size_t dst_size, const T &addr_info) {
            ssize_t rc = -1;
            u8 *cur = dst;

            {
                const u32 value = AddrInfoMagic;
                if ((rc = DNSSerializer::ToBuffer(cur, dst_size - (cur - dst), value)) == -1) {
                    return rc;
                }
                cur += rc;
            }

            {
                const u32 value = static_cast<u32>(addr_info.ai_flags);
                if ((rc = DNSSerializer::ToBuffer(cur, dst_size - (cur - dst), value)) == -1) {
                    return rc;
                }
                cur += rc;
            }

            {
                const u32 value = static_cast<u32>(addr_info.ai_family);
                if ((rc = DNSSerializer::ToBuffer(cur, dst_size - (cur - dst), value)) == -1) {
                    return rc;
                }
                cur += rc;
            }

            {
                const u32 value = static_cast<u32>(addr_info.ai_socktype);
                if ((rc = DNSSerializer::ToBuffer(cur, dst_size - (cur - dst), value)) == -1) {
                    return rc;
                }
                cur += rc;
            }

            {
                const u32 value = static_cast<u32>(addr_info.ai_protocol);
                if ((rc = DNSSerializer::ToBuffer(cur, dst_size - (cur - dst), value)) == -1) {
                    return rc;
                }
                cur += rc;
            }

            {
                const u32 value = static_cast<u32>(addr_info.ai_addrlen);
                if ((rc = DNSSerializer::ToBuffer(cur, dst_size - (cur - dst), value)) == -1) {
                    return rc;
                }
                cur += rc;
            }

            {
                if (addr_info.ai_addr == nullptr || addr_info.ai_addrlen == 0) {
                    const u32 value = 0;
                    if ((rc = DNSSerializer::ToBuffer(cur, dst_size - (cur - dst), value)) == -1) {
                        return rc;
                    }
                } else if (IsAfInet<T>(addr_info.ai_family)) {
                    if ((rc = DNSSerializer::ToBuffer(cur, dst_size - (cur - dst), *reinterpret_cast<SockAddrInType<T> *>(addr_info.ai_addr))) == -1) {
                        return rc;
                    }
                } else if (IsAfInet6<T>(addr_info.ai_family)) {
                    if ((rc = DNSSerializer::ToBuffer(cur, dst_size - (cur - dst), *reinterpret_cast<SockAddrIn6Type<T> *>(addr_info.ai_addr))) == -1) {
                        return rc;
                    }
                } else {
                    if (dst_size - (cur - dst) < addr_info.ai_addrlen) {
                        rc = -1;
                        return rc;
                    }

                    /* NOTE: This is clearly a nintendo bug, see the accompanying note in FromBufferInternalImpl */
                    std::memmove(cur, std::addressof(addr_info.ai_addr), addr_info.ai_addrlen);
                    rc = addr_info.ai_addrlen;
                }
                cur += rc;
            }

            {
                if ((rc = DNSSerializer::ToBuffer(cur, dst_size - (cur - dst), addr_info.ai_canonname)) == -1) {
                    return rc;
                }
                cur += rc;
            }

            if (addr_info.ai_next == nullptr) {
                const u32 value = 0;
                if ((rc = DNSSerializer::ToBuffer(cur, dst_size - (cur - dst), value)) == -1) {
                    return rc;
                }
                cur += rc;
            }

            rc = cur - dst;
            return rc;
        }

        template<typename T> requires IsAddrInfo<T>
        ssize_t ToBufferImpl(u8 * const dst, size_t dst_size, const T &in) {
            ssize_t rc = -1;
            u8 *cur = dst;
            std::memset(dst, 0, dst_size);

            const size_t required = DNSSerializer::SizeOf(in);
            if ((rc = DNSSerializer::CheckToBufferArguments(cur, dst_size, required, __LINE__)) == -1) {
                return rc;
            }

            for (const T *addr_info = std::addressof(in); addr_info != nullptr; addr_info = addr_info->ai_next) {
                if ((rc = ToBufferInternalImpl(cur, dst_size, *addr_info)) == -1) {
                    return rc;
                }
                cur += rc;
            }

            rc = cur - dst;
            return rc;
        }

        template<typename T> requires IsAddrInfo<T>
        ssize_t FromBufferInternalImpl(T &out, const u8 *src, size_t src_size) {
            ssize_t rc = -1;
            const u8 *cur = src;

            std::memset(std::addressof(out), 0, sizeof(out));

            ON_SCOPE_EXIT { if (rc < 0) { FreeAddrInfo(out); } };

            u32 tmp_value;

            {
                if ((rc = DNSSerializer::FromBuffer(tmp_value, cur, src_size - (cur - src))) == -1) {
                    return rc;
                } else if (tmp_value != AddrInfoMagic) {
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

            {
                if ((rc = DNSSerializer::FromBuffer(tmp_value, cur, src_size - (cur - src))) == -1) {
                    return rc;
                }
                out.ai_addrlen = static_cast<decltype(out.ai_addrlen)>(tmp_value);
                cur += rc;
            }

            {
                if (out.ai_addrlen == 0) {
                    if ((rc = DNSSerializer::FromBuffer(tmp_value, cur, src_size - (cur - src))) == -1) {
                        return rc;
                    }

                    if (tmp_value != 0) {
                        rc = -1;
                        return rc;
                    }
                    out.ai_addr = nullptr;
                } else if (IsAfInet<T>(out.ai_family)) {
                    out.ai_addr = static_cast<SockAddrType<T> *>(ams::socket::impl::Alloc(sizeof(SockAddrInType<T>)));
                    if (out.ai_addr == nullptr) {
                        rc = -1;
                        return rc;
                    }
                    std::memset(out.ai_addr, 0, sizeof(SockAddrInType<T>));

                    if ((rc = DNSSerializer::FromBuffer(*reinterpret_cast<SockAddrInType<T> *>(out.ai_addr), cur, src_size - (cur - src))) == -1) {
                        return rc;
                    }
                } else if (IsAfInet6<T>(out.ai_family)) {
                    out.ai_addr = static_cast<SockAddrType<T> *>(ams::socket::impl::Alloc(sizeof(SockAddrIn6Type<T>)));
                    if (out.ai_addr == nullptr) {
                        rc = -1;
                        return rc;
                    }
                    std::memset(out.ai_addr, 0, sizeof(SockAddrIn6Type<T>));

                    if ((rc = DNSSerializer::FromBuffer(*reinterpret_cast<SockAddrIn6Type<T> *>(out.ai_addr), cur, src_size - (cur - src))) == -1) {
                        return rc;
                    }
                } else {
                    out.ai_addr = static_cast<decltype(out.ai_addr)>(ams::socket::impl::Alloc(out.ai_addrlen));
                    if (out.ai_addr == nullptr) {
                        rc = -1;
                        return rc;
                    }

                    /* NOTE: This is *clearly* a nintendo bug. */
                    /* They obviously intend to copy to the buffer they just allocated, but instead they copy to the addrinfo structure itself. */
                    /* Probably &out.ai_addr instead of &out.ai_addr[0]? Either way, we'll implement what they do, but... */
                    std::memcpy(std::addressof(out.ai_addr), cur, out.ai_addrlen);
                    rc = out.ai_addrlen;
                }
                cur += rc;
            }

            {
                if ((rc = DNSSerializer::FromBuffer(out.ai_canonname, cur, src_size - (cur - src))) == -1) {
                    return rc;
                }
                cur += rc;
            }

            {
                if ((rc = DNSSerializer::FromBuffer(tmp_value, cur, src_size - (cur - src))) == -1) {
                    return rc;
                } else if (tmp_value == 0) {
                    out.ai_next = nullptr;
                    cur += rc;
                } else if (tmp_value == AddrInfoMagic) {
                    out.ai_next = static_cast<T *>(ams::socket::impl::Alloc(sizeof(T)));
                    if (out.ai_next == nullptr) {
                        rc = -1;
                        return rc;
                    }

                    std::memset(out.ai_next, 0, sizeof(T));
                } else {
                    rc = -1;
                    return rc;
                }
            }

            rc = cur - src;
            return rc;
        }

        template<typename T> requires IsAddrInfo<T>
        ssize_t FromBufferImpl(T &out, const u8 *src, size_t src_size) {
            ssize_t rc = 0;
            const u8 *cur = src;

            const size_t required = DNSSerializer::SizeOf(out);
            if (src_size < required) {
                ams::socket::SetLastError(ams::socket::Errno::ENoSpc);
                rc = -1;
                return rc;
            }

            for (T *addr_info = std::addressof(out); addr_info != nullptr; addr_info = addr_info->ai_next) {
                if ((rc = FromBufferInternalImpl(*addr_info, cur, src_size - (cur - src))) == -1) {
                    rc = -1;
                    return rc;
                }
                cur += rc;
            }

            rc = cur - src;
            return rc;
        }

    }

    template<>  size_t DNSSerializer::SizeOf(const struct addrinfo &in) {
        return SizeOfImpl(in);
    }

    template<>  size_t DNSSerializer::SizeOf(const ams::socket::AddrInfo &in) {
        return SizeOfImpl(in);
    }

    template<> ssize_t DNSSerializer::ToBuffer(u8 * const dst, size_t dst_size, const ams::socket::AddrInfo &in) {
        return ToBufferImpl(dst, dst_size, in);
    }

    template<> ssize_t DNSSerializer::ToBuffer(u8 * const dst, size_t dst_size, const struct addrinfo &in) {
        return ToBufferImpl(dst, dst_size, in);
    }

    template<> ssize_t DNSSerializer::FromBuffer(ams::socket::AddrInfo &out, const u8 *src, size_t src_size) {
        return FromBufferImpl(out, src, src_size);
    }

    template<> ssize_t DNSSerializer::FromBuffer(struct addrinfo &out, const u8 *src, size_t src_size) {
        return FromBufferImpl(out, src, src_size);
    }

    void FreeAddrInfo(ams::socket::AddrInfo &addr_info) {
        return FreeAddrInfoImpl(addr_info);
    }

    void FreeAddrInfo(struct addrinfo &addr_info) {
        return FreeAddrInfoImpl(addr_info);
    }

}
