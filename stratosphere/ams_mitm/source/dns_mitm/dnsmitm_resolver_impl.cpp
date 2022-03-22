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
#include "dnsmitm_resolver_impl.hpp"
#include "dnsmitm_debug.hpp"
#include "dnsmitm_host_redirection.hpp"
#include "serializer/serializer.hpp"
#include "sfdnsres_shim.h"

namespace ams::mitm::socket::resolver {

    ssize_t SerializeRedirectedHostEnt(u8 * const dst, size_t dst_size, const char *hostname, ams::socket::InAddrT redirect_addr) {
        struct in_addr addr = { .s_addr = redirect_addr };
        struct in_addr *addr_list[2] = { std::addressof(addr), nullptr };

        struct hostent ent = {
            .h_name      = const_cast<char *>(hostname),
            .h_aliases   = nullptr,
            .h_addrtype  = AF_INET,
            .h_length    = sizeof(u32),
            .h_addr_list = (char **)addr_list,
        };

        const auto result = serializer::DNSSerializer::ToBuffer(dst, dst_size, ent);
        AMS_ABORT_UNLESS(result >= 0);
        return result;
    }

    ssize_t SerializeRedirectedAddrInfo(u8 * const dst, size_t dst_size, const char *hostname, ams::socket::InAddrT redirect_addr, u16 redirect_port, const struct addrinfo *hint) {
        AMS_UNUSED(hostname);

        struct addrinfo ai = {
            .ai_flags     = 0,
            .ai_family    = AF_UNSPEC,
            .ai_socktype  = 0,
            .ai_protocol  = 0,
            .ai_addrlen   = 0,
            .ai_canonname = nullptr,
            .ai_next      = nullptr,
        };

        if (hint != nullptr) {
            ai = *hint;
        }

        switch (ai.ai_family) {
            case AF_UNSPEC: ai.ai_family = AF_INET; break;
            case AF_INET:   ai.ai_family = AF_INET; break;
            case AF_INET6:  AMS_ABORT("Redirected INET6 not supported"); break;
            AMS_UNREACHABLE_DEFAULT_CASE();
        }

        if (ai.ai_socktype == 0) {
            ai.ai_socktype = SOCK_STREAM;
        }

        if (ai.ai_protocol == 0) {
            ai.ai_protocol = IPPROTO_TCP;
        }

        const struct sockaddr_in sin = {
            .sin_family = AF_INET,
            .sin_port   = ams::socket::InetHtons(redirect_port),
            .sin_addr   = { .s_addr = redirect_addr },
            .sin_zero   = {},
        };

        ai.ai_addrlen = sizeof(sin);
        ai.ai_addr    = (struct sockaddr *)(std::addressof(sin));

        const auto result = serializer::DNSSerializer::ToBuffer(dst, dst_size, ai);
        AMS_ABORT_UNLESS(result >= 0);
        return result;
    }

    Result ResolverImpl::GetHostByNameRequest(u32 cancel_handle, const sf::ClientProcessId &client_pid, bool use_nsd_resolve, const sf::InBuffer &name, sf::Out<u32> out_host_error, sf::Out<u32> out_errno, const sf::OutBuffer &out_hostent, sf::Out<u32> out_size) {
        AMS_UNUSED(cancel_handle, client_pid, use_nsd_resolve);

        const char *hostname = reinterpret_cast<const char *>(name.GetPointer());

        LogDebug("[%016lx]: GetHostByNameRequest(%s)\n", m_client_info.program_id.value, hostname);

        R_UNLESS(hostname != nullptr, sm::mitm::ResultShouldForwardToSession());

        ams::socket::InAddrT redirect_addr = {};
        R_UNLESS(GetRedirectedHostByName(std::addressof(redirect_addr), hostname), sm::mitm::ResultShouldForwardToSession());

        LogDebug("[%016lx]: Redirecting %s to %u.%u.%u.%u\n", m_client_info.program_id.value, hostname, (redirect_addr >> 0) & 0xFF, (redirect_addr >> 8) & 0xFF, (redirect_addr >> 16) & 0xFF, (redirect_addr >> 24) & 0xFF);
        const auto size = SerializeRedirectedHostEnt(out_hostent.GetPointer(), out_hostent.GetSize(), hostname, redirect_addr);

        *out_host_error = 0;
        *out_errno      = 0;
        *out_size       = size;

        return ResultSuccess();
    }

    Result ResolverImpl::GetAddrInfoRequest(u32 cancel_handle, const sf::ClientProcessId &client_pid, bool use_nsd_resolve, const sf::InBuffer &node, const sf::InBuffer &srv, const sf::InBuffer &serialized_hint, const sf::OutBuffer &out_addrinfo, sf::Out<u32> out_errno, sf::Out<s32> out_retval, sf::Out<u32> out_size) {
        AMS_UNUSED(cancel_handle, client_pid, use_nsd_resolve);

        const char *hostname = reinterpret_cast<const char *>(node.GetPointer());

        LogDebug("[%016lx]: GetAddrInfoRequest(%s, %s)\n", m_client_info.program_id.value, reinterpret_cast<const char *>(node.GetPointer()), reinterpret_cast<const char *>(srv.GetPointer()));

        R_UNLESS(hostname != nullptr, sm::mitm::ResultShouldForwardToSession());

        ams::socket::InAddrT redirect_addr = {};
        R_UNLESS(GetRedirectedHostByName(std::addressof(redirect_addr), hostname), sm::mitm::ResultShouldForwardToSession());

        u16 port = 0;
        if (srv.GetPointer() != nullptr) {
            for (const char *cur = reinterpret_cast<const char *>(srv.GetPointer()); *cur != 0; ++cur) {
                AMS_ABORT_UNLESS(std::isdigit(static_cast<unsigned char>(*cur)));
                port *= 10;
                port += *cur - '0';
            }
        }

        LogDebug("[%016lx]: Redirecting %s:%u to %u.%u.%u.%u\n", m_client_info.program_id.value, hostname, port, (redirect_addr >> 0) & 0xFF, (redirect_addr >> 8) & 0xFF, (redirect_addr >> 16) & 0xFF, (redirect_addr >> 24) & 0xFF);

        const bool use_hint = serialized_hint.GetPointer() != nullptr;
        struct addrinfo hint = {};
        if (use_hint) {
            AMS_ABORT_UNLESS(serializer::DNSSerializer::FromBuffer(hint, serialized_hint.GetPointer(), serialized_hint.GetSize()) >= 0);
        }
        ON_SCOPE_EXIT { if (use_hint) { serializer::FreeAddrInfo(hint); } };

        const auto size = SerializeRedirectedAddrInfo(out_addrinfo.GetPointer(), out_addrinfo.GetSize(), hostname, redirect_addr, port, use_hint ? std::addressof(hint) : nullptr);

        *out_retval = 0;
        *out_errno  = 0;
        *out_size   = size;

        return ResultSuccess();
    }

    Result ResolverImpl::GetHostByNameRequestWithOptions(const sf::ClientProcessId &client_pid, const sf::InAutoSelectBuffer &name, const sf::OutAutoSelectBuffer &out_hostent, sf::Out<u32> out_size, u32 options_version, const sf::InAutoSelectBuffer &options, u32 num_options, sf::Out<s32> out_host_error, sf::Out<s32> out_errno) {
        AMS_UNUSED(client_pid, options_version, options, num_options);

        const char *hostname = reinterpret_cast<const char *>(name.GetPointer());

        LogDebug("[%016lx]: GetHostByNameRequestWithOptions(%s)\n", m_client_info.program_id.value, hostname);

        R_UNLESS(hostname != nullptr, sm::mitm::ResultShouldForwardToSession());

        ams::socket::InAddrT redirect_addr = {};
        R_UNLESS(GetRedirectedHostByName(std::addressof(redirect_addr), hostname), sm::mitm::ResultShouldForwardToSession());

        LogDebug("[%016lx]: Redirecting %s to %u.%u.%u.%u\n", m_client_info.program_id.value, hostname, (redirect_addr >> 0) & 0xFF, (redirect_addr >> 8) & 0xFF, (redirect_addr >> 16) & 0xFF, (redirect_addr >> 24) & 0xFF);
        const auto size = SerializeRedirectedHostEnt(out_hostent.GetPointer(), out_hostent.GetSize(), hostname, redirect_addr);

        *out_host_error = 0;
        *out_errno      = 0;
        *out_size       = size;

        return ResultSuccess();
    }

    Result ResolverImpl::GetAddrInfoRequestWithOptions(const sf::ClientProcessId &client_pid, const sf::InBuffer &node, const sf::InBuffer &srv, const sf::InBuffer &serialized_hint, const sf::OutAutoSelectBuffer &out_addrinfo, sf::Out<u32> out_size, sf::Out<s32> out_retval, u32 options_version, const sf::InAutoSelectBuffer &options, u32 num_options, sf::Out<s32> out_host_error, sf::Out<s32> out_errno) {
        AMS_UNUSED(client_pid, options_version, options, num_options);

        const char *hostname = reinterpret_cast<const char *>(node.GetPointer());

        LogDebug("[%016lx]: GetAddrInfoRequestWithOptions(%s, %s)\n", m_client_info.program_id.value, hostname, reinterpret_cast<const char *>(srv.GetPointer()));

        R_UNLESS(hostname != nullptr, sm::mitm::ResultShouldForwardToSession());

        ams::socket::InAddrT redirect_addr = {};
        R_UNLESS(GetRedirectedHostByName(std::addressof(redirect_addr), hostname), sm::mitm::ResultShouldForwardToSession());

        u16 port = 0;
        if (srv.GetPointer() != nullptr) {
            for (const char *cur = reinterpret_cast<const char *>(srv.GetPointer()); *cur != 0; ++cur) {
                AMS_ABORT_UNLESS(std::isdigit(static_cast<unsigned char>(*cur)));
                port *= 10;
                port += *cur - '0';
            }
        }

        LogDebug("[%016lx]: Redirecting %s:%u to %u.%u.%u.%u\n", m_client_info.program_id.value, hostname, port, (redirect_addr >> 0) & 0xFF, (redirect_addr >> 8) & 0xFF, (redirect_addr >> 16) & 0xFF, (redirect_addr >> 24) & 0xFF);

        const bool use_hint = serialized_hint.GetPointer() != nullptr;
        struct addrinfo hint = {};
        if (use_hint) {
            AMS_ABORT_UNLESS(serializer::DNSSerializer::FromBuffer(hint, serialized_hint.GetPointer(), serialized_hint.GetSize()) >= 0);
        }
        ON_SCOPE_EXIT { if (use_hint) { serializer::FreeAddrInfo(hint); } };

        const auto size = SerializeRedirectedAddrInfo(out_addrinfo.GetPointer(), out_addrinfo.GetSize(), hostname, redirect_addr, port, use_hint ? std::addressof(hint) : nullptr);

        *out_retval      = 0;
        *out_host_error  = 0;
        *out_errno       = 0;
        *out_size        = size;

        return ResultSuccess();
    }

    Result ResolverImpl::AtmosphereReloadHostsFile() {
        /* Perform a hosts file reload. */
        InitializeResolverRedirections();
        return ResultSuccess();
    }

}
