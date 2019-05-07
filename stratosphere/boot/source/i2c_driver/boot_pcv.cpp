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

#include "i2c_types.hpp"
#include "i2c_registers.hpp"
#include "boot_pcv.hpp"

static I2cBus GetI2cBus(PcvModule module) {
    switch (module) {
        case PcvModule_I2C1:
            return I2cBus_I2c1;
        case PcvModule_I2C2:
            return I2cBus_I2c2;
        case PcvModule_I2C3:
            return I2cBus_I2c3;
        case PcvModule_I2C4:
            return I2cBus_I2c4;
        case PcvModule_I2C5:
            return I2cBus_I2c5;
        case PcvModule_I2C6:
            return I2cBus_I2c6;
        default:
            std::abort();
    }
}

void Pcv::Initialize() {
    /* Don't do anything. */
}

void Pcv::Finalize() {
    /* Don't do anything. */
}

Result Pcv::SetClockRate(PcvModule module, u32 hz) {
    /* Get clock/reset registers. */
    ClkRstRegisters regs;
    regs.SetBus(GetI2cBus(module));
    /* Set clock enabled/source. */
    SetRegisterBits(regs.clk_en_reg, regs.mask);
    ReadWriteRegisterBits(regs.clk_src_reg, 0x4, 0xFF);
    svcSleepThread(1000ul);
    ReadWriteRegisterBits(regs.clk_src_reg, 0, 0xE0000000);
    svcSleepThread(2000ul);

    return ResultSuccess;
}

Result Pcv::SetClockEnabled(PcvModule module, bool enabled) {
    return ResultSuccess;
}

Result Pcv::SetVoltageEnabled(u32 domain, bool enabled) {
    return ResultSuccess;
}

Result Pcv::SetVoltageValue(u32 domain, u32 voltage) {
    return ResultSuccess;
}

Result Pcv::SetReset(PcvModule module, bool reset) {
    /* Get clock/reset registers. */
    ClkRstRegisters regs;
    regs.SetBus(GetI2cBus(module));

    /* Set/clear reset. */
    if (reset) {
        SetRegisterBits(regs.rst_reg, regs.mask);
    } else {
        ClearRegisterBits(regs.rst_reg, ~regs.mask);
    }

    return ResultSuccess;
}