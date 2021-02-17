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
#include "htcs_manager_service_object.hpp"
#include "htcs_socket_service_object.hpp"

namespace ams::htcs::server {

    #define AMS_HTCS_MANAGER_DEPRECATED_API() AMS_ABORT("Deprecated IHtcsManager API %s was called.\n", AMS_CURRENT_FUNCTION_NAME)

    Result ManagerServiceObject::Socket(sf::Out<s32> out_err, sf::Out<s32> out_sock) {
        /* NOTE: This is a deprecated API, and Nintendo aborts when it is called. */
        AMS_HTCS_MANAGER_DEPRECATED_API();
    }

    Result ManagerServiceObject::Close(sf::Out<s32> out_err, sf::Out<s32> out_res, s32 desc) {
        /* NOTE: This is a deprecated API, and Nintendo aborts when it is called. */
        AMS_HTCS_MANAGER_DEPRECATED_API();
    }

    Result ManagerServiceObject::Connect(sf::Out<s32> out_err, sf::Out<s32> out_res, s32 desc, const htcs::SockAddrHtcs &address) {
        /* NOTE: This is a deprecated API, and Nintendo aborts when it is called. */
        AMS_HTCS_MANAGER_DEPRECATED_API();
    }

    Result ManagerServiceObject::Bind(sf::Out<s32> out_err, sf::Out<s32> out_res, s32 desc, const htcs::SockAddrHtcs &address) {
        /* NOTE: This is a deprecated API, and Nintendo aborts when it is called. */
        AMS_HTCS_MANAGER_DEPRECATED_API();
    }

    Result ManagerServiceObject::Listen(sf::Out<s32> out_err, sf::Out<s32> out_res, s32 desc, s32 backlog_count) {
        /* NOTE: This is a deprecated API, and Nintendo aborts when it is called. */
        AMS_HTCS_MANAGER_DEPRECATED_API();
    }

    Result ManagerServiceObject::Accept(sf::Out<s32> out_err, sf::Out<s32> out_res, sf::Out<htcs::SockAddrHtcs> out_address, s32 desc) {
        /* NOTE: This is a deprecated API, and Nintendo aborts when it is called. */
        AMS_HTCS_MANAGER_DEPRECATED_API();
    }

    Result ManagerServiceObject::Recv(sf::Out<s32> out_err, sf::Out<s64> out_size, const sf::OutBuffer &buffer, s32 desc, s32 flags) {
        /* NOTE: This is a deprecated API, and Nintendo aborts when it is called. */
        AMS_HTCS_MANAGER_DEPRECATED_API();
    }

    Result ManagerServiceObject::Send(sf::Out<s32> out_err, sf::Out<s64> out_size, s32 desc, const sf::InBuffer &buffer, s32 flags) {
        /* NOTE: This is a deprecated API, and Nintendo aborts when it is called. */
        AMS_HTCS_MANAGER_DEPRECATED_API();
    }

    Result ManagerServiceObject::Shutdown(sf::Out<s32> out_err, sf::Out<s32> out_res, s32 desc, s32 how) {
        /* NOTE: This is a deprecated API, and Nintendo aborts when it is called. */
        AMS_HTCS_MANAGER_DEPRECATED_API();
    }

    Result ManagerServiceObject::Fcntl(sf::Out<s32> out_err, sf::Out<s32> out_res, s32 desc, s32 command, s32 value) {
        /* NOTE: This is a deprecated API, and Nintendo aborts when it is called. */
        AMS_HTCS_MANAGER_DEPRECATED_API();
    }

}
