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

#include "i2c_driver/i2c_api.hpp"

class RtcDriver {
    private:
        I2cSessionImpl i2c_session;
    public:
        RtcDriver() {
            I2cDriver::Initialize();
            I2cDriver::OpenSession(&this->i2c_session, I2cDevice_Max77620Rtc);
        }

        ~RtcDriver() {
            I2cDriver::CloseSession(this->i2c_session);
            I2cDriver::Finalize();
        }
    private:
        Result ReadRtcRegister(u8 *out, u8 address);
    public:
        Result GetRtcIntr(u8 *out);
        Result GetRtcIntrM(u8 *out);
};