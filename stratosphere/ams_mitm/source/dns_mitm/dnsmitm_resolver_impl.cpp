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
    
    namespace {
        
        struct AresAddrInfoQuery {
            std::string hostname;
            ams::socket::InAddrT redirect_addr;
            u16 redirect_port;
            bool use_hint;
            struct ams::socket::ares_addrinfo_hints hint;
        };
        
        std::map<ssize_t, std::unique_ptr<AresAddrInfoQuery>> g_ares_addr_info_queries;
        
    }

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
    
    void QueueAresAddrInfoQuery(ssize_t query_id, const char *hostname, ams::socket::InAddrT redirect_addr, u16 redirect_port, const struct ams::socket::ares_addrinfo_hints *hint) {
        AresAddrInfoQuery query;
        query.hostname = hostname;
        query.redirect_addr = redirect_addr;
        query.redirect_port = redirect_port;
        query.use_hint = hint != nullptr;
        if (query.use_hint) {
            query.hint = *hint;
        }
        
        g_ares_addr_info_queries.emplace(query_id, std::make_unique<AresAddrInfoQuery>(query));
    }
    
    ssize_t DequeueAresAddrInfoQuery(u8 * const dst, size_t dst_size, u32 query_id) {
        AresAddrInfoQuery *query = nullptr;
        
        for (auto it = g_ares_addr_info_queries.begin(); it != g_ares_addr_info_queries.end(); ++it) {
            if (it->first == static_cast<ssize_t>(query_id)) {
                query = it->second.get();
                break;
            }
        }
        
        if (query == nullptr) {
            return -1;
        }
        
        struct ams::socket::ares_addrinfo_node node = {
            .ai_ttl      = 0,
            .ai_flags    = AF_UNSPEC,
            .ai_family   = 0,
            .ai_socktype = 0,
            .ai_protocol = 0,
            .ai_addrlen  = 0,
            .ai_next     = nullptr,
        };
        
        if (query->use_hint) {
            std::memcpy(std::addressof(node.ai_flags), std::addressof(query->hint), sizeof(query->hint));
        }

        switch (node.ai_family) {
            case AF_UNSPEC: node.ai_family = AF_INET; break;
            case AF_INET:   node.ai_family = AF_INET; break;
            case AF_INET6:  AMS_ABORT("Redirected INET6 not supported"); break;
            AMS_UNREACHABLE_DEFAULT_CASE();
        }

        if (node.ai_socktype == 0) {
            node.ai_socktype = SOCK_STREAM;
        }

        if (node.ai_protocol == 0) {
            node.ai_protocol = IPPROTO_TCP;
        }

        const struct sockaddr_in sin = {
            .sin_family = AF_INET,
            .sin_port   = ams::socket::InetHtons(query->redirect_port),
            .sin_addr   = { .s_addr = query->redirect_addr },
            .sin_zero   = {},
        };

        node.ai_addrlen = sizeof(sin);
        node.ai_addr    = (struct sockaddr *)(std::addressof(sin));
        
        struct ams::socket::ares_addrinfo ai {
            .cnames = nullptr,
            .nodes  = std::addressof(node),
            .name   = const_cast<char *>(query->hostname.c_str()),
        };

        const auto result = serializer::DNSSerializer::ToBuffer(dst, dst_size, ai);
        AMS_ABORT_UNLESS(result >= 0);
        g_ares_addr_info_queries.erase(static_cast<ssize_t>(query_id));
        return result;
    }
    
    bool CancelAresAddrInfoQuery(u32 query_id) {
        for (auto it = g_ares_addr_info_queries.begin(); it != g_ares_addr_info_queries.end(); ++it) {
            if (it->first == static_cast<ssize_t>(query_id)) {
                g_ares_addr_info_queries.erase(it);
                return true;
            }
        }
        
        return false;
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

        R_SUCCEED();
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

        R_SUCCEED();
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

        R_SUCCEED();
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

        R_SUCCEED();
    }
    
    Result ResolverImpl::ReportTelemetry(const sf::ClientProcessId &client_pid, const sf::InBuffer &telemetry, u32 version) {
        /* NOTE: As of 23.0.0 this command does nothing. We mitm it anyway for future proofing. */
        AMS_UNUSED(client_pid, telemetry, version);
        R_SUCCEED();
    }
    
    Result ResolverImpl::QueryAddrInfoRequestWithOptions(const sf::ClientProcessId &client_pid, const sf::InAutoSelectBuffer &node, const sf::InAutoSelectBuffer &srv, const sf::InAutoSelectBuffer &serialized_hint, sf::Out<u32> out_query_id, u32 fd, u32 options_version, const sf::InAutoSelectBuffer &options, u32 num_options, sf::Out<s32> out_retval, sf::Out<s32> out_errno) {
        const char *hostname = reinterpret_cast<const char *>(node.GetPointer());

        LogDebug("[%016lx]: QueryAddrInfoRequestWithOptions(%s, %s)\n", m_client_info.program_id.value, hostname, reinterpret_cast<const char *>(srv.GetPointer()));

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

        LogDebug("[%016lx]: Queueing query for redirecting %s:%u to %u.%u.%u.%u\n", m_client_info.program_id.value, hostname, port, (redirect_addr >> 0) & 0xFF, (redirect_addr >> 8) & 0xFF, (redirect_addr >> 16) & 0xFF, (redirect_addr >> 24) & 0xFF);
        
        const bool use_hint = serialized_hint.GetPointer() != nullptr;
        struct ams::socket::ares_addrinfo_hints hint = {};
        if (use_hint) {
            AMS_ABORT_UNLESS(serializer::DNSSerializer::FromBuffer(hint, serialized_hint.GetPointer(), serialized_hint.GetSize()) >= 0);
        }
        
        /* Forward to session. The provided socket EventFd must be signalled by having bsdsocket write 1 to the associated socket. */
        /* This is fine though, we only care about replacing with our redirection during fetch. */
        /* Additionally, a failure at this point can be safely handled by bsdsocket, it simply won't create a new query. */
        R_TRY(sfdnsresQueryAddrInfoRequestWithOptionsFwd(m_forward_service.get(), static_cast<u64>(client_pid.GetValue().value), node.GetPointer(), node.GetSize(), srv.GetPointer(), srv.GetSize(), serialized_hint.GetPointer(), serialized_hint.GetSize(), reinterpret_cast<u32 *>(out_query_id.GetPointer()), fd, options_version, options.GetPointer(), options.GetSize(), num_options, reinterpret_cast<s32 *>(out_retval.GetPointer()), reinterpret_cast<s32 *>(out_errno.GetPointer())));
        
        /* If no errors were returned, trust the id bsdsocket generated and queue the query. */
        /* When bsdsocket's queue is full, retval and errno will reflect that and the returned query id will be -1. */
        /* This is expected by the client. We must drop this query and let the client see the errors for it to schedule a new query. */
        if ((*out_retval == 0) && (*out_errno == 0)) {
            ssize_t query_id = static_cast<ssize_t>(*out_query_id);
            QueueAresAddrInfoQuery(query_id, hostname, redirect_addr, port, use_hint ? std::addressof(hint) : nullptr);
            LogDebug("[%016lx]: Queued query with id 0x%lx\n", m_client_info.program_id.value, query_id);
        } else {
            LogDebug("[%016lx]: Dropping last query, queue is full\n", m_client_info.program_id.value);
        }
        
        R_SUCCEED();
    }
    
    Result ResolverImpl::FetchAddrInfoRequestWithOptions(const sf::ClientProcessId &client_pid, u32 query_id, u32 version, const sf::OutAutoSelectBuffer &out_addrinfo, sf::Out<u32> out_size, sf::Out<s32> out_retval, sf::Out<s32> out_errno) {
        LogDebug("[%016lx]: FetchAddrInfoRequestWithOptions(0x%x)\n", m_client_info.program_id.value, query_id);
        
        const auto size = DequeueAresAddrInfoQuery(out_addrinfo.GetPointer(), out_addrinfo.GetSize(), query_id);

        /* If we don't have this query queued, then the hostname is not in the hosts file. Forward to session. */
        R_UNLESS(size >= 0, sm::mitm::ResultShouldForwardToSession());
        
        LogDebug("[%016lx]: Dequeued query 0x%x with size 0x%lx\n", m_client_info.program_id.value, query_id, size);
        
        /* Forward to session. The socket EventFd associated with this query must be cleared by having bsdsocket write 0 to the socket. */
        /* However, we want to keep our redirection, so we pass nullptr to bsdsocket forcing the command to end early with an error. */
        /* We discard the error and replace the output, pretending the command was successful while returning our redirected addrinfo to the client instead. */ 
        R_DISCARD(sfdnsresFetchAddrInfoRequestWithOptionsFwd(m_forward_service.get(), static_cast<u64>(client_pid.GetValue().value), query_id, version, nullptr, 0, reinterpret_cast<u32 *>(out_size.GetPointer()), reinterpret_cast<s32 *>(out_retval.GetPointer()), reinterpret_cast<s32 *>(out_errno.GetPointer())));
        
        *out_size        = size;
        *out_retval      = 0;
        *out_errno       = 0;

        R_SUCCEED();
    }
    
    Result ResolverImpl::CancelQuery(const sf::ClientProcessId &client_pid, u32 query_id) {
        LogDebug("[%016lx]: CancelQuery(0x%x)\n", m_client_info.program_id.value, query_id);
        
        const auto found = CancelAresAddrInfoQuery(query_id);
        
        /* If we don't have this query queued, then we never intercepted it. Forward to session. */
        R_UNLESS(found, sm::mitm::ResultShouldForwardToSession());
        
        LogDebug("[%016lx]: Cancelled query 0x%x\n", m_client_info.program_id.value, query_id);
        
        /* Forward to session. Ensures that bsdsocket cleans up. */
        /* We still want to discard any errors and return success in case this was a query we had queued. */
        R_DISCARD(sfdnsresCancelQueryFwd(m_forward_service.get(), static_cast<u64>(client_pid.GetValue().value), query_id));

        R_SUCCEED();
    }

    Result ResolverImpl::AtmosphereReloadHostsFile() {
        /* Perform a hosts file reload. */
        InitializeResolverRedirections();
        R_SUCCEED();
    }

}
