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
#include "htcs_manager_service_object.hpp"
#include "htcs_socket_service_object.hpp"

namespace ams::htcs::server {

    #define AMS_HTCS_MANAGER_DEPRECATED_API(...) ({ AMS_UNUSED(__VA_ARGS__); AMS_ABORT("Deprecated IHtcsManager API %s was called.\n", AMS_CURRENT_FUNCTION_NAME); })

    Result SocketServiceObject::Accept(sf::Out<s32> out_err, sf::Out<sf::SharedPointer<tma::ISocket>> out, sf::Out<htcs::SockAddrHtcs> out_address) {
        /* NOTE: This is a deprecated API, and Nintendo aborts when it is called. */
        AMS_HTCS_MANAGER_DEPRECATED_API(out_err, out, out_address);
    }

}
