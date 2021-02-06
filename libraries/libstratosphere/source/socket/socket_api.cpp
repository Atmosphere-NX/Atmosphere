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
#include "impl/socket_api.hpp"

namespace ams::socket {

    Errno GetLastError() {
        return impl::GetLastError();
    }

    void SetLastError(Errno err) {
        return impl::SetLastError(err);
    }

    u32 InetHtonl(u32 host) {
        return impl::InetHtonl(host);
    }

    u16 InetHtons(u16 host) {
        return impl::InetHtons(host);
    }

    u32 InetNtohl(u32 net) {
        return impl::InetNtohl(net);
    }

    u16 InetNtohs(u16 net) {
        return impl::InetNtohs(net);
    }

}
