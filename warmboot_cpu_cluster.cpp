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
#include "warmboot_clkrst.hpp"
#include "warmboot_util.hpp"

namespace ams::warmboot {

    namespace {

        constexpr inline const uintptr_t APB_MISC  = secmon::MemoryRegionPhysicalDeviceApbMisc.GetAddress();
        constexpr inline const uintptr_t CLKRST    = secmon::MemoryRegionPhysicalDeviceClkRst.GetAddress();
        constexpr inline const uintptr_t FLOW_CTLR = secmon::MemoryRegionPhysicalDeviceFlowController.GetAddress();
        constexpr inline const uintptr_t GPIO      = secmon::MemoryRegionPhysicalDeviceGpio.GetAddress();
        constexpr inline const uintptr_t MSELECT   = MSELECT(0);
        constexpr inline const uintptr_t PMC       = secmon::MemoryRegionPhysicalDevicePmc.GetAddress();
        constexpr inline const uintptr_t SYSTEM    = secmon::MemoryRegionPhysicalDeviceSystem.GetAddress();

        ALWAYS_INLINE void EnableClusterPartition(const reg::BitsValue value, APBDEV_PMC_PWRGATE_TOGGLE_PARTID part_id) {
            /* Toggle the partitions if necessary. */
            if (!reg::HasValue(PMC + APBDEV_PMC_PWRGATE_STATUS, value)) {
                reg::Write(PMC + APBDEV_PMC_PWRGATE_TOGGLE, PMC_REG_BITS_ENUM (PWRGATE_TOGGLE_START,   ENABLE),
                                                            PMC_REG_BITS_VALUE(PWRGATE_TOGGLE_PARTID, part_id));
            }

            /* Wait for the toggle to complete. */
            while (!reg::HasValue(PMC + APBDEV_PMC_PWRGATE_STATUS, value)) { /* ... */ }

            /* Remove clamping. */
            reg::Write(PMC + APBDEV_PMC_REMOVE_CLAMPING_CMD, value);

            /* Wait for clamping to be removed. */
            while (reg::HasValue(PMC + APBDEV_PMC_CLAMP_STATUS, value)) { /* ... */ }
        }

    }

    void InitializeCpuCluster() {
        /* Set CoreSight reset. */
        reg::Write(CLKRST + CLK_RST_CONTROLLER_RST_DEV_U_SET, CLK_RST_REG_BITS_ENUM(RST_DEV_U_SET_SET_CSITE_RST, ENABLE));

        /* Restore PROD setting to CPU_SOFTRST_CTRL2 by clearing CAR2PMC_CPU_ACK_WIDTH. */
        reg::ReadWrite(CLKRST + CLK_RST_CONTROLLER_CPU_SOFTRST_CTRL2, CLK_RST_REG_BITS_VALUE(CPU_SOFTRST_CTRL2_CAR2PMC_CPU_ACK_WIDTH, 0));

        /* Restore the CPU reset vector from PMC scratch. */
        reg::Write(SYSTEM + SB_AA64_RESET_LOW,  reg::Read(PMC + APBDEV_PMC_SECURE_SCRATCH34) | 1);
        reg::Write(SYSTEM + SB_AA64_RESET_HIGH, reg::Read(PMC + APBDEV_PMC_SECURE_SCRATCH35));

        /* Configure SUPER_CCLKG_DIVIDER. */
        reg::Write(CLKRST + CLK_RST_CONTROLLER_SUPER_CCLKG_DIVIDER, CLK_RST_REG_BITS_ENUM (SUPER_CCLKG_DIVIDER_SUPER_CDIV_ENB,                ENABLE),
                                                                    CLK_RST_REG_BITS_ENUM (SUPER_CCLK_DIVIDER_SUPER_CDIV_DIS_FROM_COP_FIQ, NO_IMPACT),
                                                                    CLK_RST_REG_BITS_ENUM (SUPER_CCLK_DIVIDER_SUPER_CDIV_DIS_FROM_CPU_FIQ, NO_IMPACT),
                                                                    CLK_RST_REG_BITS_ENUM (SUPER_CCLK_DIVIDER_SUPER_CDIV_DIS_FROM_COP_IRQ, NO_IMPACT),
                                                                    CLK_RST_REG_BITS_ENUM (SUPER_CCLK_DIVIDER_SUPER_CDIV_DIS_FROM_CPU_IRQ, NO_IMPACT),
                                                                    CLK_RST_REG_BITS_VALUE(SUPER_CCLK_DIVIDER_SUPER_CDIV_DIVIDEND,                 0),
                                                                    CLK_RST_REG_BITS_VALUE(SUPER_CCLK_DIVIDER_SUPER_CDIV_DIVISOR,                  0));

        /* Configure SUPER_CCLKLP_DIVIDER. */
        reg::Write(CLKRST + CLK_RST_CONTROLLER_SUPER_CCLKLP_DIVIDER, CLK_RST_REG_BITS_ENUM (SUPER_CCLKLP_DIVIDER_SUPER_CDIV_ENB,               ENABLE),
                                                                     CLK_RST_REG_BITS_ENUM (SUPER_CCLK_DIVIDER_SUPER_CDIV_DIS_FROM_COP_FIQ, NO_IMPACT),
                                                                     CLK_RST_REG_BITS_ENUM (SUPER_CCLK_DIVIDER_SUPER_CDIV_DIS_FROM_CPU_FIQ, NO_IMPACT),
                                                                     CLK_RST_REG_BITS_ENUM (SUPER_CCLK_DIVIDER_SUPER_CDIV_DIS_FROM_COP_IRQ, NO_IMPACT),
                                                                     CLK_RST_REG_BITS_ENUM (SUPER_CCLK_DIVIDER_SUPER_CDIV_DIS_FROM_CPU_IRQ, NO_IMPACT),
                                                                     CLK_RST_REG_BITS_VALUE(SUPER_CCLK_DIVIDER_SUPER_CDIV_DIVIDEND,                 0),
                                                                     CLK_RST_REG_BITS_VALUE(SUPER_CCLK_DIVIDER_SUPER_CDIV_DIVISOR,                  0));

        /* Enable CoreSight clock. */
        reg::Write(CLKRST + CLK_RST_CONTROLLER_CLK_ENB_U_SET, CLK_RST_REG_BITS_ENUM(CLK_ENB_U_SET_SET_CLK_ENB_CSITE, ENABLE));

        /* Clear CoreSight reset. */
        reg::Write(CLKRST + CLK_RST_CONTROLLER_RST_DEV_U_CLR, CLK_RST_REG_BITS_ENUM(RST_DEV_U_CLR_CLR_CSITE_RST, ENABLE));

        /* Select MSELECT clock source as PLLP_OUT0 with divider of 4. */
        reg::Write(CLKRST + CLK_RST_CONTROLLER_CLK_SOURCE_MSELECT, CLK_RST_REG_BITS_ENUM (CLK_SOURCE_MSELECT_MSELECT_CLK_SRC,     PLLP_OUT0),
                                                                   CLK_RST_REG_BITS_VALUE(CLK_SOURCE_MSELECT_MSELECT_CLK_DIVISOR,         6));

        /* Enable clock to MSELECT. */
        reg::Write(CLKRST + CLK_RST_CONTROLLER_CLK_ENB_V_SET, CLK_RST_REG_BITS_ENUM(CLK_ENB_V_SET_SET_CLK_ENB_MSELECT, ENABLE));

        /* Wait two microseconds, then take MSELECT out of reset. */
        util::WaitMicroSeconds(2);
        reg::Write(CLKRST + CLK_RST_CONTROLLER_RST_DEV_V_CLR, CLK_RST_REG_BITS_ENUM(RST_DEV_V_CLR_CLR_MSELECT_RST, ENABLE));

        /* Workaround bug by disabling MSELECT error mechanism and enabling WRAP type conversion. */
        reg::ReadWrite(MSELECT + MSELECT_CONFIG, MSELECT_REG_BITS_ENUM(CONFIG_ERR_RESP_EN_SLAVE1,  DISABLE),
                                                 MSELECT_REG_BITS_ENUM(CONFIG_ERR_RESP_EN_SLAVE2,  DISABLE),
                                                 MSELECT_REG_BITS_ENUM(CONFIG_WRAP_TO_INCR_SLAVE0,  ENABLE),
                                                 MSELECT_REG_BITS_ENUM(CONFIG_WRAP_TO_INCR_SLAVE1,  ENABLE),
                                                 MSELECT_REG_BITS_ENUM(CONFIG_WRAP_TO_INCR_SLAVE2,  ENABLE));

        /* Disable PLLX. */
        reg::ReadWrite(CLKRST + CLK_RST_CONTROLLER_PLLX_BASE, CLK_RST_REG_BITS_ENUM(PLLX_BASE_PLLX_ENABLE, DISABLE));

        /* Clear bit 0 of PMC Scratch 190. */
        reg::ReadWrite(PMC + APBDEV_PMC_SCRATCH190, REG_BITS_VALUE(0, 1, 0));

        /* Clear PMC_DPD_SAMPLE, and wait 10 us for clear to take effect. */
        reg::Write(PMC + APBDEV_PMC_DPD_SAMPLE, 0);
        util::WaitMicroSeconds(10);

        /* Configure UART2_RTS low (GPIO controller 2 G). */
        reg::ReadWrite(GPIO + 0x108, REG_BITS_VALUE(2, 1, 1)); /* GPIO_CNF */
        reg::ReadWrite(GPIO + 0x118, REG_BITS_VALUE(2, 1, 1)); /* GPIO_OE  */
        reg::ReadWrite(GPIO + 0x128, REG_BITS_VALUE(2, 1, 0)); /* GPIO_OUT */

        /* Tristate CLDVFS PWN. */
        reg::Write(APB_MISC + PINMUX_AUX_DVFS_PWM, PINMUX_REG_BITS_ENUM(AUX_TRISTATE,    TRISTATE),
                                                   PINMUX_REG_BITS_ENUM(AUX_DVFS_PWM_PM,   CLDVFS));
        reg::Read(APB_MISC + PINMUX_AUX_DVFS_PWM);

        /* Restore PWR_I2C E_INPUT. */
        reg::Write(APB_MISC + PINMUX_AUX_PWR_I2C_SCL, PINMUX_REG_BITS_ENUM(AUX_E_INPUT, ENABLE));
        reg::Write(APB_MISC + PINMUX_AUX_PWR_I2C_SDA, PINMUX_REG_BITS_ENUM(AUX_E_INPUT, ENABLE));

        /* Enable CLDVFS clock. */
        reg::Write(CLKRST + CLK_RST_CONTROLLER_CLK_ENB_W_SET, CLK_RST_REG_BITS_ENUM(CLK_ENB_W_SET_SET_CLK_ENB_DVFS, ENABLE));

        /* Set CLDVFS clock source/divider. */
        reg::Write(CLKRST + CLK_RST_CONTROLLER_CLK_SOURCE_DVFS_REF, CLK_RST_REG_BITS_ENUM (CLK_SOURCE_DVFS_REF_DVFS_REF_CLK_SRC, PLLP_OUT0),
                                                                    CLK_RST_REG_BITS_VALUE(CLK_SOURCE_DVFS_REF_DVFS_REF_DIVISOR,        14));
        reg::Write(CLKRST + CLK_RST_CONTROLLER_CLK_SOURCE_DVFS_SOC, CLK_RST_REG_BITS_ENUM (CLK_SOURCE_DVFS_SOC_DVFS_SOC_CLK_SRC, PLLP_OUT0),
                                                                    CLK_RST_REG_BITS_VALUE(CLK_SOURCE_DVFS_SOC_DVFS_SOC_DIVISOR,        14));

        /* Enable PWR_I2C controller (I2C5). */
        reg::Write(CLKRST + CLK_RST_CONTROLLER_CLK_ENB_H_SET,   CLK_RST_REG_BITS_ENUM(CLK_ENB_H_SET_SET_CLK_ENB_I2C5, ENABLE));
        reg::Write(CLKRST + CLK_RST_CONTROLLER_RST_DEV_H_SET,   CLK_RST_REG_BITS_ENUM(RST_DEV_H_SET_SET_I2C5_RST,     ENABLE));
        util::WaitMicroSeconds(5);

        reg::Write(CLKRST + CLK_RST_CONTROLLER_CLK_SOURCE_I2C5, CLK_RST_REG_BITS_ENUM (CLK_SOURCE_I2C5_I2C5_CLK_SRC,     PLLP_OUT0),
                                                                CLK_RST_REG_BITS_VALUE(CLK_SOURCE_I2C5_I2C5_CLK_DIVISOR,         4));

        reg::Write(CLKRST + CLK_RST_CONTROLLER_RST_DEV_H_CLR,   CLK_RST_REG_BITS_ENUM(RST_DEV_H_CLR_CLR_I2C5_RST,     ENABLE));

        /* Set the EN bit in pmic regulator. */
        pmic::SetEnBit(fuse::GetRegulator());

        /* Wait 2ms. */
        util::WaitMicroSeconds(2'000);

        /* Enable power to the CRAIL partition. */
        EnableClusterPartition(PMC_REG_BITS_ENUM(PWRGATE_STATUS_CRAIL, ON), APBDEV_PMC_PWRGATE_TOGGLE_PARTID_CRAIL);

        /* Remove software clamp to CRAIL. */
        reg::Write(PMC + APBDEV_PMC_SET_SW_CLAMP, 0);
        reg::Write(PMC + APBDEV_PMC_REMOVE_CLAMPING_CMD, PMC_REG_BITS_ENUM(REMOVE_CLAMPING_COMMAND_CRAIL, ENABLE));
        while (reg::HasValue(PMC + APBDEV_PMC_CLAMP_STATUS, PMC_REG_BITS_ENUM(CLAMP_STATUS_CRAIL, ENABLE))) { /* ... */ }

        /* Spinloop 8 times, to add a little delay. */
        SpinLoop(8);

        /* Disable PWR_I2C controller (I2C5). */
        reg::Write(CLKRST + CLK_RST_CONTROLLER_RST_DEV_H_SET,   CLK_RST_REG_BITS_ENUM(RST_DEV_H_SET_SET_I2C5_RST,     ENABLE));
        reg::Write(CLKRST + CLK_RST_CONTROLLER_CLK_ENB_H_CLR,   CLK_RST_REG_BITS_ENUM(CLK_ENB_H_CLR_CLR_CLK_ENB_I2C5, ENABLE));

        /* Disable CLDVFS clock. */
        reg::Write(CLKRST + CLK_RST_CONTROLLER_CLK_ENB_W_CLR, CLK_RST_REG_BITS_ENUM(CLK_ENB_W_CLR_CLR_CLK_ENB_DVFS, ENABLE));

        /* Perform fast cluster RAM repair if needed. */
        if (!reg::HasValue(FLOW_CTLR + FLOW_CTLR_BPMP_CLUSTER_CONTROL, FLOW_REG_BITS_ENUM(BPMP_CLUSTER_CONTROL_ACTIVE_CLUSTER, SLOW))) {
            reg::Write(FLOW_CTLR + FLOW_CTLR_RAM_REPAIR, FLOW_REG_BITS_ENUM(RAM_REPAIR_REQ, ENABLE));

            while (!reg::HasValue(FLOW_CTLR + FLOW_CTLR_RAM_REPAIR, FLOW_REG_BITS_ENUM(RAM_REPAIR_STS, DONE))) {
                /* ... */
            }
        }

        /* Enable power to the non-cpu partition. */
        EnableClusterPartition(PMC_REG_BITS_ENUM(PWRGATE_STATUS_C0NC, ON), APBDEV_PMC_PWRGATE_TOGGLE_PARTID_C0NC);

        /* Enable clock to PLLP_OUT_CPU. */
        reg::Write(CLKRST + CLK_RST_CONTROLLER_CLK_ENB_Y_SET, CLK_RST_REG_BITS_ENUM(CLK_ENB_Y_SET_SET_CLK_ENB_PLLP_OUT_CPU, ENABLE));
        util::WaitMicroSeconds(2);

        /* Enable clock to the cpu complex. */
        reg::Write(CLKRST + CLK_RST_CONTROLLER_CLK_ENB_L_SET, CLK_RST_REG_BITS_ENUM(CLK_ENB_L_SET_SET_CLK_ENB_CPU,  ENABLE));
        reg::Write(CLKRST + CLK_RST_CONTROLLER_CLK_ENB_V_SET, CLK_RST_REG_BITS_ENUM(CLK_ENB_V_SET_SET_CLK_ENB_CPUG, ENABLE));
        util::WaitMicroSeconds(10);

        /* Select cpu complex clock source. */
        reg::Write(CLKRST + CLK_RST_CONTROLLER_CCLKG_BURST_POLICY,  CLK_RST_REG_BITS_ENUM(CCLK_BURST_POLICY_CWAKEUP_IDLE_SOURCE, PLLP_OUT0),
                                                                    CLK_RST_REG_BITS_ENUM(CCLK_BURST_POLICY_CWAKEUP_RUN_SOURCE,  PLLP_OUT0),
                                                                    CLK_RST_REG_BITS_ENUM(CCLK_BURST_POLICY_CWAKEUP_IRQ_SOURCE,  PLLP_OUT0),
                                                                    CLK_RST_REG_BITS_ENUM(CCLK_BURST_POLICY_CWAKEUP_FIQ_SOURCE,  PLLP_OUT0),
                                                                    CLK_RST_REG_BITS_ENUM(CCLK_BURST_POLICY_CPU_STATE,                 RUN));

        reg::Write(CLKRST + CLK_RST_CONTROLLER_CCLKLP_BURST_POLICY, CLK_RST_REG_BITS_ENUM(CCLK_BURST_POLICY_CWAKEUP_IDLE_SOURCE, PLLP_OUT0),
                                                                    CLK_RST_REG_BITS_ENUM(CCLK_BURST_POLICY_CWAKEUP_RUN_SOURCE,  PLLP_OUT0),
                                                                    CLK_RST_REG_BITS_ENUM(CCLK_BURST_POLICY_CWAKEUP_IRQ_SOURCE,  PLLP_OUT0),
                                                                    CLK_RST_REG_BITS_ENUM(CCLK_BURST_POLICY_CWAKEUP_FIQ_SOURCE,  PLLP_OUT0),
                                                                    CLK_RST_REG_BITS_ENUM(CCLK_BURST_POLICY_CPU_STATE,                 RUN));
        util::WaitMicroSeconds(10);

        /* Wake non-cpu out of reset. */
        reg::Write(CLKRST + CLK_RST_CONTROLLER_RST_CPUG_CMPLX_CLR, CLK_RST_REG_BITS_ENUM(RST_CPUG_CMPLX_CLR_CLR_NONCPURESET, ENABLE));
    }

    void PowerOnCpu() {
        /* Enable power to the CE0 partition. */
        EnableClusterPartition(PMC_REG_BITS_ENUM(PWRGATE_STATUS_CE0, ON), APBDEV_PMC_PWRGATE_TOGGLE_PARTID_CE0);

        /* Clear CPU reset. */
        reg::Write(CLKRST + CLK_RST_CONTROLLER_RST_CPUG_CMPLX_CLR, CLK_RST_REG_BITS_ENUM(RST_CPUG_CMPLX_CLR_CLR_CPURESET0,  ENABLE),
                                                                   CLK_RST_REG_BITS_ENUM(RST_CPUG_CMPLX_CLR_CLR_CORERESET0, ENABLE));
    }

}
