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
#include "boot_i2c_utils.hpp"

namespace ams::boot {

    class RtcDriver {
        private:
            i2c::driver::I2cSession i2c_session;
        public:
            RtcDriver() {
                R_ABORT_UNLESS(i2c::driver::OpenSession(std::addressof(this->i2c_session), i2c::DeviceCode_Max77620Rtc));
            }

            ~RtcDriver() {
                i2c::driver::CloseSession(this->i2c_session);
            }
        private:
            Result ReadRtcRegister(u8 *out, u8 address);
        public:
            Result GetRtcIntr(u8 *out);
            Result GetRtcIntrM(u8 *out);
    };

}
