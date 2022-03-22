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

namespace ams::dmnt::transport {

    void InitializeByHtcs();
    void InitializeByTcp();

    enum PortName {
        PortName_GdbServer,
        PortName_GdbDebugLog,
    };

    s32 Socket();
    s32 Close(s32 desc);
    s32 Bind(s32 desc, PortName port_name);
    s32 Listen(s32 desc, s32 backlog_count);
    s32 Accept(s32 desc);
    s32 Shutdown(s32 desc);

    ssize_t Recv(s32 desc, void *buffer, size_t buffer_size, s32 flags);
    ssize_t Send(s32 desc, const void *buffer, size_t buffer_size, s32 flags);

    s32 GetLastError();
    bool IsLastErrorEAgain();

}