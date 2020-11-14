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
#include "sc7fw_util.hpp"
#include "sc7fw_dram.hpp"

namespace ams::sc7fw {

    namespace {

        constexpr inline const uintptr_t PMC = secmon::MemoryRegionPhysicalDevicePmc.GetAddress();

        void DisableCrail() {
            /* Wait for CRAIL to be off. */
            while (!reg::HasValue(PMC + APBDEV_PMC_PWRGATE_STATUS, PMC_REG_BITS_ENUM(PWRGATE_STATUS_CRAIL, OFF))) { /* ... */ }

            /* Set CRAIL to be clamped. */
            reg::ReadWrite(PMC + APBDEV_PMC_SET_SW_CLAMP, PMC_REG_BITS_VALUE(SET_SW_CLAMP_CRAIL, 1));

            /* Wait for CRAIL to be clamped. */
            while (!reg::HasValue(PMC + APBDEV_PMC_CLAMP_STATUS, PMC_REG_BITS_ENUM(CLAMP_STATUS_CRAIL, ENABLE))) { /* ... */ }

            /* Spin loop for a short while, to allow time for the clamp to take effect. */
            sc7fw::SpinLoop(10);

            /* Initialize i2c-5. */
            i2c::Initialize(i2c::Port_5);

            /* Disable the voltage to CPU. */
            pmic::DisableVddCpu(fuse::GetRegulator());

            /* Wait 700 microseconds to ensure voltage is disabled. */
            util::WaitMicroSeconds(700);
        }

        void DisableAllInterrupts() {
            /* Disable all interrupts for bpmp in all interrupt controllers. */
            reg::Write(PRI_ICTLR(ICTLR_COP_IER_CLR),   ~0u);
            reg::Write(SEC_ICTLR(ICTLR_COP_IER_CLR),   ~0u);
            reg::Write(TRI_ICTLR(ICTLR_COP_IER_CLR),   ~0u);
            reg::Write(QUAD_ICTLR(ICTLR_COP_IER_CLR),  ~0u);
            reg::Write(PENTA_ICTLR(ICTLR_COP_IER_CLR), ~0u);
            reg::Write(HEXA_ICTLR(ICTLR_COP_IER_CLR),  ~0u);
        }

        void EnterSc7() {
            /* Disable read buffering and write buffering in the BPMP cache. */
            reg::ReadWrite(AVP_CACHE_ADDRESS(AVP_CACHE_CONFIG), AVP_CACHE_REG_BITS_ENUM(DISABLE_WB, TRUE),
                                                                AVP_CACHE_REG_BITS_ENUM(DISABLE_RB, TRUE));

            /* Ensure the CPU Rail is turned off. */
            DisableCrail();

            /* Disable all interrupts. */
            DisableAllInterrupts();

            /* Save the EMC FSP */
            SaveEmcFsp();

            /* Enable self-refresh for DRAM */
            EnableSdramSelfRefresh();

            /* Enable refresh for all EMC devices. */
            EnableEmcAllSegmentsRefresh();

            /* Enable deep power-down for ddr. */
            EnableDdrDeepPowerDown();

            /* Enable pad sampling during deep sleep. */
            reg::ReadWrite(PMC + APBDEV_PMC_DPD_SAMPLE, PMC_REG_BITS_ENUM(DPD_SAMPLE_ON, ENABLE));
            reg::Read(PMC + APBDEV_PMC_DPD_SAMPLE);

            /* Wait a while for pad sampling to be enabled. */
            sc7fw::SpinLoop(0x128);

            /* Enter deep sleep. */
            reg::ReadWrite(PMC + APBDEV_PMC_DPD_ENABLE, PMC_REG_BITS_ENUM(DPD_ENABLE_ON, ENABLE));

            /* Wait forever until we're asleep. */
            AMS_INFINITE_LOOP();
        }

    }

    void Main() {
        EnterSc7();
    }

    NORETURN void ExceptionHandler() {
        /* Write enable to MAIN_RESET. */
        reg::Write(PMC + APBDEV_PMC_CNTRL, PMC_REG_BITS_ENUM(CNTRL_MAIN_RESET, ENABLE));

        /* Wait forever until we're reset. */
        AMS_INFINITE_LOOP();
    }

}

namespace ams::diag {

    void AbortImpl() {
        sc7fw::ExceptionHandler();
    }

    #include <exosphere/diag/diag_detailed_assertion_impl.inc>

}
