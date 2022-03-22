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
#include <exosphere.hpp>

namespace ams::clkrst {

    namespace {

        constinit uintptr_t g_register_address = secmon::MemoryRegionPhysicalDeviceClkRst.GetAddress();

        constinit BpmpClockRate g_bpmp_clock_rate = BpmpClockRate_408MHz;

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
                .index          = CLK_RST_CONTROLLER_CLK_ENB_##_NAME_##_INDEX,                         \
                .clk_src        = CLK_RST_CONTROLLER_CLK_SOURCE_##_NAME_##_##_NAME_##_CLK_SRC_##_CLK_, \
                .clk_div        = _DIV_,                                                               \
            }

        #define DEFINE_CLOCK_PARAMETERS_WITHOUT_CLKDIV(_VARNAME_, _REG_, _NAME_)                       \
            constexpr inline const ClockParameters _VARNAME_ = {                                       \
                .reset_offset   = CLK_RST_CONTROLLER_RST_DEVICES_##_REG_,                              \
                .clk_enb_offset = CLK_RST_CONTROLLER_CLK_OUT_ENB_##_REG_,                              \
                .clk_src_offset = 0,                                                                   \
                .index          = CLK_RST_CONTROLLER_CLK_ENB_##_NAME_##_INDEX,                         \
                .clk_src        = 0,                                                                   \
                .clk_div        = 0,                                                                   \
            }

        DEFINE_CLOCK_PARAMETERS(UartAClock,  L,  UARTA, PLLP_OUT0, 0);
        DEFINE_CLOCK_PARAMETERS(UartBClock,  L,  UARTB, PLLP_OUT0, 0);
        DEFINE_CLOCK_PARAMETERS(UartCClock,  H,  UARTC, PLLP_OUT0, 0);
        DEFINE_CLOCK_PARAMETERS(I2c1Clock,   L,   I2C1,     CLK_M, 0);
        DEFINE_CLOCK_PARAMETERS(I2c5Clock,   H,   I2C5,     CLK_M, 0);
        DEFINE_CLOCK_PARAMETERS(SeClock,     V,     SE, PLLP_OUT0, 0);
        DEFINE_CLOCK_PARAMETERS(ActmonClock, V, ACTMON,     CLK_M, 0);

        DEFINE_CLOCK_PARAMETERS(CsiteClock,  U,  CSITE, PLLP_OUT0, 4);
        DEFINE_CLOCK_PARAMETERS(Host1xClock, L, HOST1X, PLLP_OUT0, 3);
        DEFINE_CLOCK_PARAMETERS(TsecClock,   U,   TSEC, PLLP_OUT0, 2);
        DEFINE_CLOCK_PARAMETERS(Sor1Clock,   X,   SOR1, PLLP_OUT0, 2);

        DEFINE_CLOCK_PARAMETERS_WITHOUT_CLKDIV(CldvfsClock, W, DVFS);

        DEFINE_CLOCK_PARAMETERS_WITHOUT_CLKDIV(TzramClock,  V, TZRAM);

        DEFINE_CLOCK_PARAMETERS_WITHOUT_CLKDIV(SorSafeClock, Y, SOR_SAFE);
        DEFINE_CLOCK_PARAMETERS_WITHOUT_CLKDIV(Sor0Clock,    X, SOR0);
        DEFINE_CLOCK_PARAMETERS_WITHOUT_CLKDIV(KfuseClock,   H, KFUSE);

        DEFINE_CLOCK_PARAMETERS_WITHOUT_CLKDIV(Cache2Clock, L, CACHE2);
        DEFINE_CLOCK_PARAMETERS_WITHOUT_CLKDIV(Cram2Clock, U, CRAM2);

        constexpr const u32 PllcDivn[] = {
            [BpmpClockRate_408MHz] =  0,
            [BpmpClockRate_544MHz] = 85,
            [BpmpClockRate_576MHz] = 90,
            [BpmpClockRate_589MHz] = 92,
        };

        void EnablePllc(BpmpClockRate rate) {
            const u32 desired_divn = PllcDivn[rate];

            /* Check if we're already enabled. */
            const bool is_enabled   = reg::HasValue(g_register_address + CLK_RST_CONTROLLER_PLLC_BASE, CLK_RST_REG_BITS_ENUM(PLLC_BASE_PLLC_ENABLE, ENABLE));
            const bool is_good_divn = reg::HasValue(g_register_address + CLK_RST_CONTROLLER_PLLC_BASE, CLK_RST_REG_BITS_VALUE(PLLC_BASE_PLLC_DIVN, desired_divn));
            if (is_enabled && is_good_divn) {
                return;
            }

            /* Take PLLC out of reset. */
            reg::Write(g_register_address + CLK_RST_CONTROLLER_PLLC_MISC, (reg::Read(g_register_address + CLK_RST_CONTROLLER_PLLC_MISC) & 0xBFF0000F) | (0x80000 << 4));
            reg::SetBits(g_register_address + CLK_RST_CONTROLLER_PLLC_MISC2, 0xF0 << 8);

            /* Disable pll. */
            reg::ReadWrite(g_register_address + CLK_RST_CONTROLLER_PLLC_BASE, CLK_RST_REG_BITS_ENUM(PLLC_BASE_PLLC_ENABLE, DISABLE));
            reg::ClearBits(g_register_address + CLK_RST_CONTROLLER_PLLC_MISC1, (1u << 27));
            util::WaitMicroSeconds(10);

            /* Set dividers. */
            reg::Write(g_register_address + CLK_RST_CONTROLLER_PLLC_BASE, CLK_RST_REG_BITS_VALUE(PLLC_BASE_PLLC_DIVM,            4),
                                                                          CLK_RST_REG_BITS_VALUE(PLLC_BASE_PLLC_DIVN, desired_divn));

            /* Enable pll. */
            reg::ReadWrite(g_register_address + CLK_RST_CONTROLLER_PLLC_BASE, CLK_RST_REG_BITS_ENUM(PLLC_BASE_PLLC_ENABLE, ENABLE));
            while (!reg::HasValue(g_register_address + CLK_RST_CONTROLLER_PLLC_BASE, CLK_RST_REG_BITS_ENUM(PLLC_BASE_PLLC_LOCK, LOCK))) {
                /* ... */
            }

            /* Disable PLLC_OUT1. */
            reg::Write(g_register_address + CLK_RST_CONTROLLER_PLLC_OUT, CLK_RST_REG_BITS_VALUE(PLLC_OUT_PLLC_OUT1_RATIO, 1));

            /* Enable PLLC_OUT1. */
            reg::ReadWrite(g_register_address + CLK_RST_CONTROLLER_PLLC_OUT, CLK_RST_REG_BITS_ENUM(PLLC_OUT_PLLC_OUT1_RSTN,  RESET_DISABLE),
                                                                             CLK_RST_REG_BITS_ENUM(PLLC_OUT_PLLC_OUT1_CLKEN,        ENABLE));
            util::WaitMicroSeconds(1'000);
        }

        void DisablePllc() {
            /* Disable PLLC/PLLC_OUT1. */
            reg::ReadWrite(g_register_address + CLK_RST_CONTROLLER_PLLC_OUT, CLK_RST_REG_BITS_ENUM(PLLC_OUT_PLLC_OUT1_RSTN,  RESET_ENABLE),
                                                                             CLK_RST_REG_BITS_ENUM(PLLC_OUT_PLLC_OUT1_CLKEN,      DISABLE));

            reg::ReadWrite(g_register_address + CLK_RST_CONTROLLER_PLLC_BASE, CLK_RST_REG_BITS_ENUM(PLLC_BASE_PLLC_ENABLE, DISABLE));
            reg::ReadWrite(g_register_address + CLK_RST_CONTROLLER_PLLC_BASE, CLK_RST_REG_BITS_ENUM(PLLC_BASE_PLLC_REF_DIS, REF_DISABLE));
            reg::SetBits(g_register_address + CLK_RST_CONTROLLER_PLLC_MISC1, (1u << 27));
            reg::SetBits(g_register_address + CLK_RST_CONTROLLER_PLLC_MISC, (1u << 30));
            util::WaitMicroSeconds(10);
        }

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

    void EnableSeClock() {
        EnableClock(SeClock);
        if (fuse::GetSocType() == fuse::SocType_Mariko) {
            reg::ReadWrite(g_register_address + CLK_RST_CONTROLLER_CLK_SOURCE_SE, CLK_RST_REG_BITS_ENUM(CLK_SOURCE_SE_CLK_LOCK, ENABLE));
        }
    }

    void EnableCldvfsClock() {
        EnableClock(CldvfsClock);
    }

    void EnableCsiteClock() {
        EnableClock(CsiteClock);
    }

    void EnableTzramClock() {
        EnableClock(TzramClock);
    }

    void EnableCache2Clock() {
        EnableClock(Cache2Clock);
    }

    void EnableCram2Clock() {
        EnableClock(Cram2Clock);
    }

    void EnableHost1xClock() {
        EnableClock(Host1xClock);
    }

    void EnableTsecClock() {
        EnableClock(TsecClock);
    }

    void EnableSorSafeClock() {
        EnableClock(SorSafeClock);
    }

    void EnableSor0Clock() {
        EnableClock(Sor0Clock);
    }

    void EnableSor1Clock() {
        EnableClock(Sor1Clock);
    }

    void EnableKfuseClock() {
        EnableClock(KfuseClock);
    }

    void DisableI2c1Clock() {
        DisableClock(I2c1Clock);
    }

    void DisableHost1xClock() {
        DisableClock(Host1xClock);
    }

    void DisableTsecClock() {
        DisableClock(TsecClock);
    }

    void DisableSorSafeClock() {
        DisableClock(SorSafeClock);
    }

    void DisableSor0Clock() {
        DisableClock(Sor0Clock);
    }

    void DisableSor1Clock() {
        DisableClock(Sor1Clock);
    }

    void DisableKfuseClock() {
        DisableClock(KfuseClock);
    }

    BpmpClockRate GetBpmpClockRate() {
        return g_bpmp_clock_rate;
    }

    BpmpClockRate SetBpmpClockRate(BpmpClockRate rate) {
        /* Get the current rate. */
        const auto prev_rate = g_bpmp_clock_rate;

        /* Cap our rate. */
        if (rate >= BpmpClockRate_Count) {
            rate = BpmpClockRate_589MHz;
        }

        /* Change the rate, if we need to. */
        if (rate == prev_rate) {
            return prev_rate;
        }

        /* Configure the rate. */
        if (rate != BpmpClockRate_408MHz) {
            /* If we were previously overclocked, restore to PLLP_OUT. */
            if (prev_rate != BpmpClockRate_408MHz) {
                reg::Write(g_register_address + CLK_RST_CONTROLLER_SCLK_BURST_POLICY, CLK_RST_REG_BITS_ENUM(SCLK_BURST_POLICY_SYS_STATE,                       RUN),
                                                                                      CLK_RST_REG_BITS_ENUM(SCLK_BURST_POLICY_COP_AUTO_SWAKEUP_FROM_FIQ,       NOP),
                                                                                      CLK_RST_REG_BITS_ENUM(SCLK_BURST_POLICY_CPU_AUTO_SWAKEUP_FROM_FIQ,       NOP),
                                                                                      CLK_RST_REG_BITS_ENUM(SCLK_BURST_POLICY_COP_AUTO_SWAKEUP_FROM_IRQ,       NOP),
                                                                                      CLK_RST_REG_BITS_ENUM(SCLK_BURST_POLICY_CPU_AUTO_SWAKEUP_FROM_IRQ,       NOP),
                                                                                      CLK_RST_REG_BITS_ENUM(SCLK_BURST_POLICY_SWAKEUP_FIQ_SOURCE,        PLLP_OUT0),
                                                                                      CLK_RST_REG_BITS_ENUM(SCLK_BURST_POLICY_SWAKEUP_IRQ_SOURCE,        PLLP_OUT0),
                                                                                      CLK_RST_REG_BITS_ENUM(SCLK_BURST_POLICY_SWAKEUP_RUN_SOURCE,        PLLP_OUT0),
                                                                                      CLK_RST_REG_BITS_ENUM(SCLK_BURST_POLICY_SWAKEUP_IDLE_SOURCE,       PLLP_OUT0));
                util::WaitMicroSeconds(1'000);
            }

            /* Configure PLLC. */
            EnablePllc(rate);

            /* Set SCLK. */
            reg::Write(g_register_address + CLK_RST_CONTROLLER_CLK_SYSTEM_RATE, CLK_RST_REG_BITS_VALUE(CLK_SYSTEM_RATE_HCLK_DIS, 0),
                                                                                CLK_RST_REG_BITS_VALUE(CLK_SYSTEM_RATE_AHB_RATE, 0),
                                                                                CLK_RST_REG_BITS_VALUE(CLK_SYSTEM_RATE_PCLK_DIS, 0),
                                                                                CLK_RST_REG_BITS_VALUE(CLK_SYSTEM_RATE_APB_RATE, 3));

            reg::Write(g_register_address + CLK_RST_CONTROLLER_SCLK_BURST_POLICY, CLK_RST_REG_BITS_ENUM(SCLK_BURST_POLICY_SYS_STATE,                       RUN),
                                                                                  CLK_RST_REG_BITS_ENUM(SCLK_BURST_POLICY_COP_AUTO_SWAKEUP_FROM_FIQ,       NOP),
                                                                                  CLK_RST_REG_BITS_ENUM(SCLK_BURST_POLICY_CPU_AUTO_SWAKEUP_FROM_FIQ,       NOP),
                                                                                  CLK_RST_REG_BITS_ENUM(SCLK_BURST_POLICY_COP_AUTO_SWAKEUP_FROM_IRQ,       NOP),
                                                                                  CLK_RST_REG_BITS_ENUM(SCLK_BURST_POLICY_CPU_AUTO_SWAKEUP_FROM_IRQ,       NOP),
                                                                                  CLK_RST_REG_BITS_ENUM(SCLK_BURST_POLICY_SWAKEUP_FIQ_SOURCE,        PLLP_OUT0),
                                                                                  CLK_RST_REG_BITS_ENUM(SCLK_BURST_POLICY_SWAKEUP_IRQ_SOURCE,        PLLP_OUT0),
                                                                                  CLK_RST_REG_BITS_ENUM(SCLK_BURST_POLICY_SWAKEUP_RUN_SOURCE,        PLLC_OUT1),
                                                                                  CLK_RST_REG_BITS_ENUM(SCLK_BURST_POLICY_SWAKEUP_IDLE_SOURCE,            CLKM));
        } else {
            /* Configure to use PLLP_OUT0. */
            reg::Write(g_register_address + CLK_RST_CONTROLLER_SCLK_BURST_POLICY, CLK_RST_REG_BITS_ENUM(SCLK_BURST_POLICY_SYS_STATE,                       RUN),
                                                                                  CLK_RST_REG_BITS_ENUM(SCLK_BURST_POLICY_COP_AUTO_SWAKEUP_FROM_FIQ,       NOP),
                                                                                  CLK_RST_REG_BITS_ENUM(SCLK_BURST_POLICY_CPU_AUTO_SWAKEUP_FROM_FIQ,       NOP),
                                                                                  CLK_RST_REG_BITS_ENUM(SCLK_BURST_POLICY_COP_AUTO_SWAKEUP_FROM_IRQ,       NOP),
                                                                                  CLK_RST_REG_BITS_ENUM(SCLK_BURST_POLICY_CPU_AUTO_SWAKEUP_FROM_IRQ,       NOP),
                                                                                  CLK_RST_REG_BITS_ENUM(SCLK_BURST_POLICY_SWAKEUP_FIQ_SOURCE,        PLLP_OUT0),
                                                                                  CLK_RST_REG_BITS_ENUM(SCLK_BURST_POLICY_SWAKEUP_IRQ_SOURCE,        PLLP_OUT0),
                                                                                  CLK_RST_REG_BITS_ENUM(SCLK_BURST_POLICY_SWAKEUP_RUN_SOURCE,        PLLP_OUT0),
                                                                                  CLK_RST_REG_BITS_ENUM(SCLK_BURST_POLICY_SWAKEUP_IDLE_SOURCE,            CLKM));
            util::WaitMicroSeconds(1'000);

            reg::Write(g_register_address + CLK_RST_CONTROLLER_CLK_SYSTEM_RATE, CLK_RST_REG_BITS_VALUE(CLK_SYSTEM_RATE_HCLK_DIS, 0),
                                                                                CLK_RST_REG_BITS_VALUE(CLK_SYSTEM_RATE_AHB_RATE, 0),
                                                                                CLK_RST_REG_BITS_VALUE(CLK_SYSTEM_RATE_PCLK_DIS, 0),
                                                                                CLK_RST_REG_BITS_VALUE(CLK_SYSTEM_RATE_APB_RATE, 2));

            /* Disable PLLC. */
            DisablePllc();
        }

        /* Set the clock rate. */
        g_bpmp_clock_rate = rate;

        /* Return the previous rate. */
        return prev_rate;
    }

}
