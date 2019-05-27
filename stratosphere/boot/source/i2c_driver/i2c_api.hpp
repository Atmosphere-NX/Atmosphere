/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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
#include <switch.h>
#include <stratosphere.hpp>

#include "i2c_types.hpp"
#include "i2c_command_list.hpp"

class I2cDriver {
    public:
        static void Initialize();
        static void Finalize();
        static void OpenSession(I2cSessionImpl *out_session, I2cDevice device);
        static void CloseSession(I2cSessionImpl &session);
        static Result Send(I2cSessionImpl &session, const void *src, size_t size, I2cTransactionOption option);
        static Result Receive(I2cSessionImpl &session, void *dst, size_t size, I2cTransactionOption option);
        static Result ExecuteCommandList(I2cSessionImpl &session, void *dst, size_t size, const void *cmd_list, size_t cmd_list_size);

        static void SuspendBuses();
        static void ResumeBuses();
        static void SuspendPowerBus();
        static void ResumePowerBus();
};