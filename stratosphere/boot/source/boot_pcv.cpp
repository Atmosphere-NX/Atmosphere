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
#include "i2c/i2c_types.hpp"
#include "i2c/driver/impl/i2c_pcv.hpp"
#include "i2c/driver/impl/i2c_registers.hpp"

using namespace ams::i2c::driver::impl;

namespace ams::pcv {

    void Initialize() {
        /* Don't do anything. */
    }

    void Finalize() {
        /* Don't do anything. */
    }

    Result SetClockRate(PcvModule module, u32 hz) {
        /* Get clock/reset registers. */
        ClkRstRegisters regs;
        regs.SetBus(ConvertFromPcvModule(module));
        /* Set clock enabled/source. */
        reg::SetBits(regs.clk_en_reg, regs.mask);
        reg::ReadWrite(regs.clk_src_reg, 0x4, 0xFF);
        svcSleepThread(1000ul);
        reg::ReadWrite(regs.clk_src_reg, 0, 0xE0000000);
        svcSleepThread(2000ul);

        return ResultSuccess();
    }

    Result SetClockEnabled(PcvModule module, bool enabled) {
        return ResultSuccess();
    }

    Result SetVoltageEnabled(u32 domain, bool enabled) {
        return ResultSuccess();
    }

    Result SetVoltageValue(u32 domain, u32 voltage) {
        return ResultSuccess();
    }

    Result SetReset(PcvModule module, bool reset) {
        /* Get clock/reset registers. */
        ClkRstRegisters regs;
        regs.SetBus(ConvertFromPcvModule(module));

        /* Set/clear reset. */
        if (reset) {
            reg::SetBits(regs.rst_reg, regs.mask);
        } else {
            reg::ClearBits(regs.rst_reg, regs.mask);
        }

        return ResultSuccess();
    }

}
