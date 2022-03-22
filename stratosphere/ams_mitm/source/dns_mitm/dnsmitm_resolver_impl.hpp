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
#pragma once
#include <stratosphere.hpp>

#define AMS_DNS_MITM_INTERFACE_INFO(C, H)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        \
    AMS_SF_METHOD_INFO(C, H,     2, Result, GetHostByNameRequest,            (u32 cancel_handle, const sf::ClientProcessId &client_pid, bool use_nsd_resolve, const sf::InBuffer &name, sf::Out<u32> out_host_error, sf::Out<u32> out_errno, const sf::OutBuffer &out_hostent, sf::Out<u32> out_size),                                                                                                                                       (cancel_handle, client_pid, use_nsd_resolve, name, out_host_error, out_errno, out_hostent, out_size))                                                               \
    AMS_SF_METHOD_INFO(C, H,     6, Result, GetAddrInfoRequest,              (u32 cancel_handle, const sf::ClientProcessId &client_pid, bool use_nsd_resolve, const sf::InBuffer &node, const sf::InBuffer &srv, const sf::InBuffer &serialized_hint, const sf::OutBuffer &out_addrinfo, sf::Out<u32> out_errno, sf::Out<s32> out_retval, sf::Out<u32> out_size),                                                                            (cancel_handle, client_pid, use_nsd_resolve, node, srv, serialized_hint, out_addrinfo, out_errno, out_retval, out_size))                                            \
    AMS_SF_METHOD_INFO(C, H,    10, Result, GetHostByNameRequestWithOptions, (const sf::ClientProcessId &client_pid, const sf::InAutoSelectBuffer &name, const sf::OutAutoSelectBuffer &out_hostent, sf::Out<u32> out_size, u32 options_version, const sf::InAutoSelectBuffer &options, u32 num_options, sf::Out<s32> out_host_error, sf::Out<s32> out_errno),                                                                               (client_pid, name, out_hostent, out_size, options_version, options, num_options, out_host_error, out_errno),                                    hos::Version_5_0_0) \
    AMS_SF_METHOD_INFO(C, H,    12, Result, GetAddrInfoRequestWithOptions,   (const sf::ClientProcessId &client_pid, const sf::InBuffer &node, const sf::InBuffer &srv, const sf::InBuffer &serialized_hint, const sf::OutAutoSelectBuffer &out_addrinfo, sf::Out<u32> out_size, sf::Out<s32> out_retval, u32 options_version, const sf::InAutoSelectBuffer &options, u32 num_options, sf::Out<s32> out_host_error, sf::Out<s32> out_errno), (client_pid, node, srv, serialized_hint, out_addrinfo, out_size, out_retval, options_version, options, num_options, out_host_error, out_errno), hos::Version_5_0_0) \
    AMS_SF_METHOD_INFO(C, H, 65000, Result, AtmosphereReloadHostsFile,       (),                                                                                                                                                                                                                                                                                                                                                             ())

AMS_SF_DEFINE_MITM_INTERFACE(ams::mitm::socket::resolver, IResolver, AMS_DNS_MITM_INTERFACE_INFO)

namespace ams::mitm::socket::resolver {

    class ResolverImpl : public sf::MitmServiceImplBase {
        public:
            using MitmServiceImplBase::MitmServiceImplBase;
        public:
            static bool ShouldMitm(const sm::MitmProcessInfo &client_info) {
                /* We will mitm:
                 * - everything.
                 */
                AMS_UNUSED(client_info);
                return true;
            }
        public:
            /* Overridden commands. */
            Result GetHostByNameRequest(u32 cancel_handle, const sf::ClientProcessId &client_pid, bool use_nsd_resolve, const sf::InBuffer &name, sf::Out<u32> out_host_error, sf::Out<u32> out_errno, const sf::OutBuffer &out_hostent, sf::Out<u32> out_size);
            Result GetAddrInfoRequest(u32 cancel_handle, const sf::ClientProcessId &client_pid, bool use_nsd_resolve, const sf::InBuffer &node, const sf::InBuffer &srv, const sf::InBuffer &serialized_hint, const sf::OutBuffer &out_addrinfo, sf::Out<u32> out_errno, sf::Out<s32> out_retval, sf::Out<u32> out_size);
            Result GetHostByNameRequestWithOptions(const sf::ClientProcessId &client_pid, const sf::InAutoSelectBuffer &name, const sf::OutAutoSelectBuffer &out_hostent, sf::Out<u32> out_size, u32 options_version, const sf::InAutoSelectBuffer &options, u32 num_options, sf::Out<s32> out_host_error, sf::Out<s32> out_errno);
            Result GetAddrInfoRequestWithOptions(const sf::ClientProcessId &client_pid, const sf::InBuffer &node, const sf::InBuffer &srv, const sf::InBuffer &serialized_hint, const sf::OutAutoSelectBuffer &out_addrinfo, sf::Out<u32> out_size, sf::Out<s32> out_retval, u32 options_version, const sf::InAutoSelectBuffer &options, u32 num_options, sf::Out<s32> out_host_error, sf::Out<s32> out_errno);

            /* Extension commands. */
            Result AtmosphereReloadHostsFile();
    };
    static_assert(IsIResolver<ResolverImpl>);

}
