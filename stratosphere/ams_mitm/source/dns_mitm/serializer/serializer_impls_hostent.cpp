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
        concept IsHostEnt = std::same_as<T, ams::socket::HostEnt> || std::same_as<T, struct hostent>;

        template<typename T> requires IsHostEnt<T>
        using InAddrType = typename std::conditional<std::same_as<T, ams::socket::HostEnt>, ams::socket::InAddr, struct in_addr>::type;

        template<typename T> requires IsHostEnt<T>
        constexpr bool IsAfInet(const auto h_addrtype) {
            if constexpr (std::same_as<T, ams::socket::HostEnt>) {
                return h_addrtype == ams::socket::Family::Af_Inet;
            } else {
                return h_addrtype == AF_INET;
            }
        }

        template<typename T> requires IsHostEnt<T>
        constexpr bool IsAfInet6(const auto h_addrtype) {
            if constexpr (std::same_as<T, ams::socket::HostEnt>) {
                return h_addrtype == ams::socket::Family::Af_Inet6;
            } else {
                return h_addrtype == AF_INET;
            }
        }

        template<typename T> requires IsHostEnt<T>
        size_t SizeOfImpl(const T &in) {
            size_t rc = 0;
            u32 dummy = 0;

            rc += DNSSerializer::SizeOf((const char *)(in.h_name));
            rc += DNSSerializer::SizeOf((const char **)(in.h_aliases), dummy);
            rc += sizeof(u32);
            rc += sizeof(u32);
            rc += DNSSerializer::SizeOf((const InAddrType<T> **)(in.h_addr_list), dummy);

            return rc;
        }

        template<typename T> requires IsHostEnt<T>
        ssize_t ToBufferImpl(u8 * const dst, size_t dst_size, const T &in) {
            ssize_t rc = -1;
            u8 *cur = dst;

            const size_t required = DNSSerializer::SizeOf(in);
            if ((rc = DNSSerializer::CheckToBufferArguments(cur, dst_size, required, __LINE__)) == -1) {
                return rc;
            }

            if ((rc = DNSSerializer::ToBuffer(cur, dst_size - (cur - dst), in.h_name)) == -1) {
                return rc;
            }
            cur += rc;

            if ((rc = DNSSerializer::ToBuffer(cur, dst_size - (cur - dst), in.h_aliases)) == -1) {
                return rc;
            }
            cur += rc;

            const u16 h_addrtype = static_cast<u16>(in.h_addrtype);
            if ((rc = DNSSerializer::ToBuffer(cur, dst_size - (cur - dst), h_addrtype)) == -1) {
                return rc;
            }
            cur += rc;

            const u16 h_length = in.h_length;
            if ((rc = DNSSerializer::ToBuffer(cur, dst_size - (cur - dst), h_length)) == -1) {
                return rc;
            }
            cur += rc;

            if (IsAfInet<T>(in.h_addrtype)) {
                if ((rc = DNSSerializer::ToBuffer(cur, dst_size - (cur - dst), (InAddrType<T> **)(in.h_addr_list))) == -1) {
                    return rc;
                }
            } else if (IsAfInet6<T>(in.h_addrtype)) {
                if ((rc = DNSSerializer::ToBuffer(cur, dst_size - (cur - dst), (InAddrType<T> **)(in.h_addr_list))) == -1) {
                    return rc;
                }
            } else {
                const u32 null = 0;
                if ((rc = DNSSerializer::ToBuffer(cur, dst_size - (cur - dst), null)) == -1) {
                    return rc;
                }
            }
            cur += rc;

            rc = cur - dst;
            return rc;
        }

        template<typename T> requires IsHostEnt<T>
        ssize_t FromBufferImpl(T &out, const u8 *src, size_t src_size) {
            ssize_t rc = -1;
            const u8 *cur = src;

            std::memset(std::addressof(out), 0, sizeof(out));

            ON_SCOPE_EXIT {
                if (rc < 0) {
                    FreeHostent(out);
                }
            };

            if ((rc = DNSSerializer::FromBuffer(out.h_name, cur, src_size - (cur - src))) == -1) {
                return rc;
            }
            cur += rc;

            if ((rc = DNSSerializer::FromBuffer(out.h_aliases, cur, src_size - (cur - src))) == -1) {
                return rc;
            }
            cur += rc;

            u16 h_addrtype = 0;
            if ((rc = DNSSerializer::FromBuffer(h_addrtype, cur, src_size - (cur - src))) == -1) {
                return rc;
            }
            out.h_addrtype = static_cast<decltype(out.h_addrtype)>(h_addrtype);
            cur += rc;

            u16 h_length = 0;
            if ((rc = DNSSerializer::FromBuffer(h_length, cur, src_size - (cur - src))) == -1) {
                return rc;
            }
            out.h_length = h_length;
            cur += rc;

            InAddrType<T> **addrs = nullptr;
            if ((rc = DNSSerializer::FromBuffer(addrs, cur, src_size - (cur - src))) == -1) {
                return rc;
            }
            out.h_addr_list = (char **)addrs;
            cur += rc;

            rc = cur - src;
            return rc;
        }

        template<typename T> requires IsHostEnt<T>
        void FreeHostentImpl(T &ent) {
            if (ent.h_name != nullptr) {
                ams::socket::impl::Free(ent.h_name);
                ent.h_name = nullptr;
            }

            if (ent.h_aliases != nullptr) {
                u32 i = 0;
                for (char *str = ent.h_aliases[i]; str != nullptr; str = ent.h_aliases[++i]) {
                    ams::socket::impl::Free(str);
                    ent.h_aliases[i] = nullptr;
                }

                ams::socket::impl::Free(ent.h_aliases);
                ent.h_aliases = nullptr;
            }

            if (ent.h_addr_list != nullptr) {
                AMS_ASSERT(ent.h_length == sizeof(u32));
                if (ent.h_length == sizeof(u32)) {
                    auto **addr_list = reinterpret_cast<InAddrType<T> **>(ent.h_addr_list);

                    u32 i = 0;
                    for (auto *addr = addr_list[i]; addr != nullptr; addr = addr_list[++i]) {
                        ams::socket::impl::Free(addr);
                        addr_list[i] = nullptr;
                    }

                    ams::socket::impl::Free(ent.h_addr_list);
                    ent.h_addr_list = nullptr;
                }
            }

            std::memset(std::addressof(ent), 0, sizeof(ent));
        }

    }

    template<>  size_t DNSSerializer::SizeOf(const struct hostent &in) {
        return SizeOfImpl(in);
    }

    template<> ssize_t DNSSerializer::ToBuffer(u8 * const dst, size_t dst_size, const struct hostent &in) {
        return ToBufferImpl(dst, dst_size, in);
    }

    template<> ssize_t DNSSerializer::FromBuffer(struct hostent &out, const u8 *src, size_t src_size) {
        return FromBufferImpl(out, src, src_size);
    }

    template<>  size_t DNSSerializer::SizeOf(const ams::socket::HostEnt &in) {
        return SizeOfImpl(in);
    }

    template<> ssize_t DNSSerializer::ToBuffer(u8 * const dst, size_t dst_size, const ams::socket::HostEnt &in) {
        return ToBufferImpl(dst, dst_size, in);
    }

    template<> ssize_t DNSSerializer::FromBuffer(ams::socket::HostEnt &out, const u8 *src, size_t src_size) {
        return FromBufferImpl(out, src, src_size);
    }

    void FreeHostent(ams::socket::HostEnt &ent) {
        return FreeHostentImpl(ent);
    }

    void FreeHostent(struct hostent &ent) {
        return FreeHostentImpl(ent);
    }

}
