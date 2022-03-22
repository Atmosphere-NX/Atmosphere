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
#include "fusee_cpu.hpp"

namespace ams::nxboot {

    namespace {

        constexpr inline const uintptr_t CLKRST  = secmon::MemoryRegionPhysicalDeviceClkRst.GetAddress();
        constexpr inline const uintptr_t PMC     = secmon::MemoryRegionPhysicalDevicePmc.GetAddress();
        constexpr inline const uintptr_t FLOW    = secmon::MemoryRegionPhysicalDeviceFlowController.GetAddress();
        constexpr inline const uintptr_t EVP     = secmon::MemoryRegionPhysicalDeviceExceptionVectors.GetAddress();
        constexpr inline const uintptr_t SYSTEM  = secmon::MemoryRegionPhysicalDeviceSystem.GetAddress();

        bool IsPartitionPowered(u32 mask) {
            return (reg::Read(PMC + APBDEV_PMC_PWRGATE_STATUS) & mask) == mask;
        }

        void PowerOnPartition(u32 status_mask, u32 toggle_mask) {
            /* Check if the partition is already powered on. */
            if (IsPartitionPowered(status_mask)) {
                return;
            }

            /* Wait for PWRGATE_TOGGLE to be idle. */
            auto timeout = 5000;
            while (true) {
                if (reg::HasValue(PMC + APBDEV_PMC_PWRGATE_TOGGLE, PMC_REG_BITS_ENUM(PWRGATE_TOGGLE_START, DISABLE))) {
                    break;
                }

                util::WaitMicroSeconds(1);
                if ((--timeout) < 0) {
                    return;
                }
            }

            /* Toggle on the desired partition. */
            reg::SetField(toggle_mask, PMC_REG_BITS_ENUM(PWRGATE_TOGGLE_START, ENABLE));
            reg::Write(PMC + APBDEV_PMC_PWRGATE_TOGGLE, toggle_mask);

            /* Wait for the partition to be powered. */
            timeout = 5000;
            while (true) {
                if (IsPartitionPowered(status_mask)) {
                    break;
                }

                util::WaitMicroSeconds(1);
                if ((--timeout) < 0) {
                    return;
                }
            }
        }

    }

    void SetupCpu(uintptr_t entrypoint) {
        /* Set ACTIVE_CLUSTER to FAST. */
        reg::ReadWrite(FLOW + FLOW_CTLR_BPMP_CLUSTER_CONTROL, FLOW_REG_BITS_ENUM(BPMP_CLUSTER_CONTROL_ACTIVE_CLUSTER, FAST));

        /* Enable VDD_CPU. */
        pmic::EnableVddCpu(fuse::GetRegulator());

        /* Enable clock to the cpu. */
        {
            /* Initialize PllX */
            if (!reg::HasValue(CLKRST + CLK_RST_CONTROLLER_PLLX_BASE, CLK_RST_REG_BITS_ENUM(PLLX_BASE_PLLX_ENABLE, ENABLE))) {
                /* Disable IDDQ. */
                reg::ReadWrite(CLKRST + CLK_RST_CONTROLLER_PLLX_MISC3, CLK_RST_REG_BITS_VALUE(PLLX_MISC3_PLLX_IDDQ, 0));

                /* Wait two microseconds. */
                util::WaitMicroSeconds(2);

                /* Configure PLLX dividers. */
                reg::Write(CLKRST + CLK_RST_CONTROLLER_PLLX_BASE, 0x80404E02);
                reg::Write(CLKRST + CLK_RST_CONTROLLER_PLLX_BASE, 0x00404E02);

                /* Set PLLX_LOCK_ENABLE. */
                reg::ReadWrite(CLKRST + CLK_RST_CONTROLLER_PLLX_MISC, CLK_RST_REG_BITS_ENUM(PLLX_MISC_PLLX_LOCK_ENABLE, ENABLE));

                /* Enable PLLX. */
                reg::Write(CLKRST + CLK_RST_CONTROLLER_PLLX_BASE, 0x40404E02);
            }

            /* Wait for PLLX to be locked. */
            while (!reg::HasValue(CLKRST + CLK_RST_CONTROLLER_PLLX_BASE, CLK_RST_REG_BITS_ENUM(PLLX_BASE_PLLX_LOCK, LOCK))) {
                /* ... */
            }

            /* Select MSELECT clock source as PLLP_OUT0 with divider of 4. */
            reg::ReadWrite(CLKRST + CLK_RST_CONTROLLER_CLK_SOURCE_MSELECT, CLK_RST_REG_BITS_ENUM (CLK_SOURCE_MSELECT_MSELECT_CLK_SRC,     PLLP_OUT0),
                                                                           CLK_RST_REG_BITS_VALUE(CLK_SOURCE_MSELECT_MSELECT_CLK_DIVISOR,         6));

            /* Enable clock to MSELECT. */
            reg::ReadWrite(CLKRST + CLK_RST_CONTROLLER_CLK_OUT_ENB_V, CLK_RST_REG_BITS_ENUM(CLK_OUT_ENB_V_CLK_ENB_MSELECT, ENABLE));

            /* Configure CCLK_BURST_POLICY. */
            reg::Write(CLKRST + CLK_RST_CONTROLLER_CCLK_BURST_POLICY, CLK_RST_REG_BITS_ENUM(CCLK_BURST_POLICY_CWAKEUP_IDLE_SOURCE, PLLX_OUT0_LJ),
                                                                      CLK_RST_REG_BITS_ENUM(CCLK_BURST_POLICY_CWAKEUP_RUN_SOURCE,  PLLX_OUT0_LJ),
                                                                      CLK_RST_REG_BITS_ENUM(CCLK_BURST_POLICY_CWAKEUP_IRQ_SOURCE,  PLLX_OUT0_LJ),
                                                                      CLK_RST_REG_BITS_ENUM(CCLK_BURST_POLICY_CWAKEUP_FIQ_SOURCE,  PLLX_OUT0_LJ),
                                                                      CLK_RST_REG_BITS_ENUM(CCLK_BURST_POLICY_CPU_STATE,                    RUN));

            /* Configure SUPER_CCLK_DIVIDER. */
            reg::Write(CLKRST + CLK_RST_CONTROLLER_SUPER_CCLK_DIVIDER, CLK_RST_REG_BITS_ENUM (SUPER_CCLK_DIVIDER_SUPER_CDIV_ENB,                 ENABLE),
                                                                       CLK_RST_REG_BITS_ENUM (SUPER_CCLK_DIVIDER_SUPER_CDIV_DIS_FROM_COP_FIQ, NO_IMPACT),
                                                                       CLK_RST_REG_BITS_ENUM (SUPER_CCLK_DIVIDER_SUPER_CDIV_DIS_FROM_CPU_FIQ, NO_IMPACT),
                                                                       CLK_RST_REG_BITS_ENUM (SUPER_CCLK_DIVIDER_SUPER_CDIV_DIS_FROM_COP_IRQ, NO_IMPACT),
                                                                       CLK_RST_REG_BITS_ENUM (SUPER_CCLK_DIVIDER_SUPER_CDIV_DIS_FROM_CPU_IRQ, NO_IMPACT),
                                                                       CLK_RST_REG_BITS_VALUE(SUPER_CCLK_DIVIDER_SUPER_CDIV_DIVIDEND,                 0),
                                                                       CLK_RST_REG_BITS_VALUE(SUPER_CCLK_DIVIDER_SUPER_CDIV_DIVISOR,                  0));


            /* Enable CPUG. */
            reg::Write(CLKRST + CLK_RST_CONTROLLER_CLK_ENB_V_SET, CLK_RST_REG_BITS_ENUM(CLK_ENB_V_SET_SET_CLK_ENB_CPUG, ENABLE));
        }

        /* Enable coresight. */
        clkrst::EnableCsiteClock();

        /* Restore PROD setting to CPU_SOFTRST_CTRL2 by clearing CAR2PMC_CPU_ACK_WIDTH. */
        reg::ReadWrite(CLKRST + CLK_RST_CONTROLLER_CPU_SOFTRST_CTRL2, CLK_RST_REG_BITS_VALUE(CPU_SOFTRST_CTRL2_CAR2PMC_CPU_ACK_WIDTH, 0));

        /* Power on cpu rails. */
        {
            PowerOnPartition(reg::EncodeValue(PMC_REG_BITS_ENUM(PWRGATE_STATUS_CRAIL, ON)), reg::EncodeValue(PMC_REG_BITS_ENUM(PWRGATE_TOGGLE_PARTID, CRAIL)));
            PowerOnPartition(reg::EncodeValue(PMC_REG_BITS_ENUM(PWRGATE_STATUS_C0NC,  ON)), reg::EncodeValue(PMC_REG_BITS_ENUM(PWRGATE_TOGGLE_PARTID,  C0NC)));
            PowerOnPartition(reg::EncodeValue(PMC_REG_BITS_ENUM(PWRGATE_STATUS_CE0,   ON)), reg::EncodeValue(PMC_REG_BITS_ENUM(PWRGATE_TOGGLE_PARTID,   CE0)));
        }

        /* Do RAM Repair. */
        {
            reg::Write(FLOW + FLOW_CTLR_RAM_REPAIR, FLOW_REG_BITS_ENUM(RAM_REPAIR_REQ, ENABLE));

            while (!reg::HasValue(FLOW + FLOW_CTLR_RAM_REPAIR, FLOW_REG_BITS_ENUM(RAM_REPAIR_STS, DONE))) {
                /* ... */
            }
        }

        /* Configure CPU reset vector. */
        reg::Write(EVP + EVP_CPU_RESET_VECTOR, 0);

        reg::Write(SYSTEM + SB_AA64_RESET_LOW,  entrypoint | 0x1);
        reg::Write(SYSTEM + SB_AA64_RESET_HIGH, 0);
        reg::Write(SYSTEM + SB_CSR, SB_REG_BITS_ENUM(CSR_NS_RST_VEC_WR_DIS, DISABLE));
        reg::Read(SYSTEM + SB_CSR);
    }

    void StartCpu() {
        /* NOTE: Here nintendo sets CPU_STRICT_TZ_APERTURE_CHECK, which we will not set. */

        /* Clear MSELECT reset. */
        reg::ReadWrite(CLKRST + CLK_RST_CONTROLLER_RST_DEVICES_V, CLK_RST_REG_BITS_ENUM(RST_DEVICES_V_SWR_MSELECT_RST, DISABLE));

        /* Take non-cpu out of reset. */
        reg::Write(CLKRST + CLK_RST_CONTROLLER_RST_CPUG_CMPLX_CLR, CLK_RST_REG_BITS_ENUM(RST_CPUG_CMPLX_CLR_CLR_NONCPURESET, ENABLE));

        /* Clear cpu reset. */

        reg::Write(CLKRST + CLK_RST_CONTROLLER_RST_CPUG_CMPLX_CLR, CLK_RST_REG_BITS_ENUM(RST_CPUG_CMPLX_CLR_CLR_CPURESET0,  ENABLE),
                                                                   CLK_RST_REG_BITS_ENUM(RST_CPUG_CMPLX_CLR_CLR_CORERESET0, ENABLE),
                                                                   CLK_RST_REG_BITS_ENUM(RST_CPUG_CMPLX_CLR_CLR_PRESETDBG,  ENABLE),
                                                                   CLK_RST_REG_BITS_ENUM(RST_CPUG_CMPLX_CLR_CLR_L2RESET,    ENABLE));
    }

}
