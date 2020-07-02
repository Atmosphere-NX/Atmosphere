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
#include <exosphere.hpp>

namespace ams::clkrst {

    namespace {

        constinit uintptr_t g_register_address = secmon::MemoryRegionPhysicalDeviceClkRst.GetAddress();

        struct ClockParameters {
            uintptr_t reset_offset;
            uintptr_t clk_enb_offset;
            uintptr_t clk_src_offset;
            u8 index;
            u8 clk_src;
            u8 clk_div;
        };

        void EnableClock(const ClockParameters &param) {
            /* Hold reset. */
            reg::ReadWrite(g_register_address + param.reset_offset,   REG_BITS_VALUE(param.index, 1, 1));

            /* Disable clock. */
            reg::ReadWrite(g_register_address + param.clk_enb_offset, REG_BITS_VALUE(param.index, 1, 0));

            /* Set the clock source. */
            if (param.clk_src_offset != 0) {
                reg::Write(g_register_address + param.clk_src_offset, (param.clk_src << 29) | (param.clk_div << 0));
            }

            /* Enable clk. */
            reg::ReadWrite(g_register_address + param.clk_enb_offset, REG_BITS_VALUE(param.index, 1, 1));

            /* Release reset. */
            reg::ReadWrite(g_register_address + param.reset_offset,   REG_BITS_VALUE(param.index, 1, 0));
        }

        void DisableClock(const ClockParameters &param) {
            /* Hold reset. */
            reg::ReadWrite(g_register_address + param.reset_offset,   REG_BITS_VALUE(param.index, 1, 1));

            /* Disable clock. */
            reg::ReadWrite(g_register_address + param.clk_enb_offset, REG_BITS_VALUE(param.index, 1, 0));
        }

        #define DEFINE_CLOCK_PARAMETERS(_VARNAME_, _REG_, _NAME_, _CLK_, _DIV_)                        \
            constexpr inline const ClockParameters _VARNAME_ = {                                       \
                .reset_offset   = CLK_RST_CONTROLLER_RST_DEVICES_##_REG_,                              \
                .clk_enb_offset = CLK_RST_CONTROLLER_CLK_OUT_ENB_##_REG_,                              \
                .clk_src_offset = CLK_RST_CONTROLLER_CLK_SOURCE_##_NAME_,                              \
                .index          = CLK_RST_CONTROLLER_CLK_ENB_##_NAME_##_INDEX,                          \
                .clk_src        = CLK_RST_CONTROLLER_CLK_SOURCE_##_NAME_##_##_NAME_##_CLK_SRC_##_CLK_, \
                .clk_div        = _DIV_,                                                               \
            }

        DEFINE_CLOCK_PARAMETERS(UartAClock,  L,  UARTA, PLLP_OUT0, 0);
        DEFINE_CLOCK_PARAMETERS(UartBClock,  L,  UARTB, PLLP_OUT0, 0);
        DEFINE_CLOCK_PARAMETERS(UartCClock,  H,  UARTC, PLLP_OUT0, 0);
        DEFINE_CLOCK_PARAMETERS(I2c1Clock,   L,   I2C1,     CLK_M, 0);
        DEFINE_CLOCK_PARAMETERS(I2c5Clock,   H,   I2C5,     CLK_M, 0);
        DEFINE_CLOCK_PARAMETERS(ActmonClock, V, ACTMON,     CLK_M, 0);

    }

    void SetRegisterAddress(uintptr_t address) {
        g_register_address = address;
    }

    void SetFuseVisibility(bool visible) {
        reg::ReadWrite(g_register_address + CLK_RST_CONTROLLER_MISC_CLK_ENB, CLK_RST_REG_BITS_VALUE(MISC_CLK_ENB_CFG_ALL_VISIBLE, visible ? 1 : 0));
    }

    void EnableUartAClock() {
        EnableClock(UartAClock);
    }

    void EnableUartBClock() {
        EnableClock(UartBClock);
    }

    void EnableUartCClock() {
        EnableClock(UartCClock);
    }

    void EnableActmonClock() {
        EnableClock(ActmonClock);
    }

    void EnableI2c1Clock() {
        EnableClock(I2c1Clock);
    }

    void EnableI2c5Clock() {
        EnableClock(I2c5Clock);
    }

    void DisableI2c1Clock() {
        DisableClock(I2c1Clock);
    }

}
