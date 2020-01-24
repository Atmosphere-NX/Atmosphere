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
#include "../i2c_types.hpp"
#include "../i2c_command_list.hpp"

namespace ams::i2c::driver {

    struct Session {
        size_t bus_idx;
        size_t session_id;
    };

    /* Initialization. */
    void Initialize();
    void Finalize();

    /* Session management. */
    void OpenSession(Session *out_session, I2cDevice device);
    void CloseSession(Session &session);

    /* Communication. */
    Result Send(Session &session, const void *src, size_t size, I2cTransactionOption option);
    Result Receive(Session &session, void *dst, size_t size, I2cTransactionOption option);
    Result ExecuteCommandList(Session &session, void *dst, size_t size, const void *cmd_list, size_t cmd_list_size);

    /* Power management. */
    void SuspendBuses();
    void ResumeBuses();
    void SuspendPowerBus();
    void ResumePowerBus();

}
