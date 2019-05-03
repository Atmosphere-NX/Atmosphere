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

#include <switch.h>
#include <stratosphere.hpp>
#include "boot_functions.hpp"
#include "boot_pmic_driver.hpp"

void PmicDriver::ShutdownSystem() {
    if (R_FAILED(this->ShutdownSystem(false))) {
        std::abort();
    }
}

void PmicDriver::RebootSystem() {
    if (R_FAILED(this->ShutdownSystem(true))) {
        std::abort();
    }
}

Result PmicDriver::GetAcOk(bool *out) {
    u8 power_status;
    Result rc = this->GetPowerStatus(&power_status);
    if (R_FAILED(rc)) {
        return rc;
    }

    *out = (power_status & 0x02) != 0;
    return ResultSuccess;
}

Result PmicDriver::GetPowerIntr(u8 *out) {
    const u8 addr = 0x0B;
    return Boot::ReadI2cRegister(this->i2c_session, out, sizeof(*out), &addr, sizeof(addr));
}

Result PmicDriver::GetPowerStatus(u8 *out) {
    const u8 addr = 0x15;
    return Boot::ReadI2cRegister(this->i2c_session, out, sizeof(*out), &addr, sizeof(addr));
}

Result PmicDriver::GetNvErc(u8 *out) {
    const u8 addr = 0x0C;
    return Boot::ReadI2cRegister(this->i2c_session, out, sizeof(*out), &addr, sizeof(addr));
}

Result PmicDriver::ShutdownSystem(bool reboot) {
    /* TODO: Implement this. */
    std::abort();
}