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
#pragma once
#include <vapours.hpp>
#include <stratosphere/socket/socket_types.hpp>
#include <stratosphere/socket/socket_errno.hpp>

namespace ams::socket {

    Errno GetLastError();
    void SetLastError(Errno err);

    u32 InetHtonl(u32 host);
    u16 InetHtons(u16 host);
    u32 InetNtohl(u32 net);
    u16 InetNtohs(u16 net);

}
