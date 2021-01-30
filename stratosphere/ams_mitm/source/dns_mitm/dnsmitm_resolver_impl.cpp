/*
 * Copyright (c) 2018-2020 Atmosphère-NX
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

namespace ams::mitm::socket::resolver {

    Result ResolverImpl::GetHostByNameRequest(u32 cancel_handle, const sf::ClientProcessId &client_pid, bool use_nsd_resolve, const sf::InBuffer &name, sf::Out<u32> out_host_error, sf::Out<u32> out_errno, const sf::OutBuffer &out_hostent, sf::Out<u32> out_size) {
        LogDebug("[%016lx]: GetHostByNameRequest(%s)\n", this->client_info.program_id.value, reinterpret_cast<const char *>(name.GetPointer()));
        return sm::mitm::ResultShouldForwardToSession();
    }

    Result ResolverImpl::GetAddrInfoRequest(u32 cancel_handle, const sf::ClientProcessId &client_pid, bool use_nsd_resolve, const sf::InBuffer &node, const sf::InBuffer &srv, const sf::InBuffer &serialized_hint, const sf::OutBuffer &out_addrinfo, sf::Out<u32> out_errno, sf::Out<s32> out_retval, sf::Out<u32> out_size) {
        LogDebug("[%016lx]: GetAddrInfoRequest(%s, %s)\n", this->client_info.program_id.value, reinterpret_cast<const char *>(node.GetPointer()), reinterpret_cast<const char *>(srv.GetPointer()));
        return sm::mitm::ResultShouldForwardToSession();
    }

    Result ResolverImpl::GetHostByNameRequestWithOptions(const sf::ClientProcessId &client_pid, const sf::InAutoSelectBuffer &name, const sf::OutAutoSelectBuffer &out_hostent, sf::Out<u32> out_size, u32 options_version, const sf::InAutoSelectBuffer &options, u32 num_options, sf::Out<s32> out_host_error, sf::Out<s32> out_errno) {
        LogDebug("[%016lx]: GetHostByNameRequestWithOptions(%s)\n", this->client_info.program_id.value, reinterpret_cast<const char *>(name.GetPointer()));
        return sm::mitm::ResultShouldForwardToSession();
    }

    Result ResolverImpl::GetAddrInfoRequestWithOptions(const sf::ClientProcessId &client_pid, const sf::InBuffer &node, const sf::InBuffer &srv, const sf::InBuffer &serialized_hint, const sf::OutAutoSelectBuffer &out_addrinfo, sf::Out<u32> out_size, sf::Out<s32> out_retval, u32 options_version, const sf::InAutoSelectBuffer &options, u32 num_options, sf::Out<s32> out_host_error, sf::Out<s32> out_errno) {
        LogDebug("[%016lx]: GetAddrInfoRequestWithOptions(%s, %s)\n", this->client_info.program_id.value, reinterpret_cast<const char *>(node.GetPointer()), reinterpret_cast<const char *>(srv.GetPointer()));
        return sm::mitm::ResultShouldForwardToSession();
    }

}
