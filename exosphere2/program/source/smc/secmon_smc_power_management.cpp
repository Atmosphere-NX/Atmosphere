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
#include "../secmon_cpu_context.hpp"
#include "../secmon_error.hpp"
#include "secmon_smc_power_management.hpp"

namespace ams::secmon::smc {

    namespace {

        constexpr inline uintptr_t PMC     = MemoryRegionVirtualDevicePmc.GetAddress();
        constexpr inline uintptr_t CLK_RST = MemoryRegionVirtualDeviceClkRst.GetAddress();

        constinit bool g_charger_hi_z_mode_enabled = false;

        constinit const reg::BitsMask CpuPowerGateStatusMasks[NumCores] = {
            PMC_REG_BITS_MASK(PWRGATE_STATUS_CE0),
            PMC_REG_BITS_MASK(PWRGATE_STATUS_CE1),
            PMC_REG_BITS_MASK(PWRGATE_STATUS_CE2),
            PMC_REG_BITS_MASK(PWRGATE_STATUS_CE3),
        };

        constinit const APBDEV_PMC_PWRGATE_TOGGLE_PARTID CpuPowerGateTogglePartitionIds[NumCores] = {
            APBDEV_PMC_PWRGATE_TOGGLE_PARTID_CE0,
            APBDEV_PMC_PWRGATE_TOGGLE_PARTID_CE1,
            APBDEV_PMC_PWRGATE_TOGGLE_PARTID_CE2,
            APBDEV_PMC_PWRGATE_TOGGLE_PARTID_CE3,
        };

        bool IsCpuPoweredOn(const reg::BitsMask mask) {
            return reg::HasValue(PMC + APBDEV_PMC_PWRGATE_STATUS, REG_BITS_VALUE_FROM_MASK(mask, APBDEV_PMC_PWRGATE_STATUS_STATUS_ON));
        }

        void PowerOnCpu(const reg::BitsMask mask, u32 toggle_partid) {
            /* If the cpu is already on, we have nothing to do. */
            if (IsCpuPoweredOn(mask)) {
                return;
            }

            /* Wait until nothing is being powergated. */
            int timeout = 5000;
            while (true) {
                if (reg::HasValue(PMC + APBDEV_PMC_PWRGATE_TOGGLE, PMC_REG_BITS_ENUM(PWRGATE_TOGGLE_START, DISABLE))) {
                    break;
                }

                util::WaitMicroSeconds(1);
                if ((--timeout) < 0) {
                    /* NOTE: Nintendo doesn't do any error handling here... */
                    return;
                }
            }

            /* Toggle on the cpu partition. */
            reg::Write(PMC + APBDEV_PMC_PWRGATE_TOGGLE, PMC_REG_BITS_ENUM (PWRGATE_TOGGLE_START,         ENABLE),
                                                        PMC_REG_BITS_VALUE(PWRGATE_TOGGLE_PARTID, toggle_partid));

            /* Wait up to 5000 us for the powergate to complete. */
            timeout = 5000;
            while (true) {
                if (IsCpuPoweredOn(mask)) {
                    break;
                }

                util::WaitMicroSeconds(1);
                if ((--timeout) < 0) {
                    /* NOTE: Nintendo doesn't do any error handling here... */
                    return;
                }
            }
        }

        void ResetCpu(int which_core) {
            reg::Write(CLK_RST + CLK_RST_CONTROLLER_RST_CPUG_CMPLX_SET, REG_BITS_VALUE(which_core + 0x00, 1, 1),  /* CPURESETn */
                                                                        REG_BITS_VALUE(which_core + 0x10, 1, 1)); /* CORERESETn */
        }

        void StartCpu(int which_core) {
            reg::Write(CLK_RST + CLK_RST_CONTROLLER_RST_CPUG_CMPLX_CLR, REG_BITS_VALUE(which_core + 0x00, 1, 1),  /* CPURESETn */
                                                                        REG_BITS_VALUE(which_core + 0x10, 1, 1)); /* CORERESETn */
        }

    }

    SmcResult SmcPowerOffCpu(const SmcArguments &args) {
        /* TODO */
        return SmcResult::NotImplemented;
    }

    SmcResult SmcPowerOnCpu(const SmcArguments &args) {
        /* Get and validate the core to power on. */
        const int which_core = args.r[1];
        if (!(0 <= which_core && which_core < NumCores)) {
            return SmcResult::PsciInvalidParameters;
        }

        /* Ensure the core isn't already on. */
        if (IsCoreOn(which_core)) {
            return SmcResult::PsciAlreadyOn;
        }

        /* Save the entry context. */
        SetEntryContext(which_core, args.r[2], args.r[3]);

        /* Reset the cpu. */
        ResetCpu(which_core);

        /* Turn on the core. */
        PowerOnCpu(CpuPowerGateStatusMasks[which_core], CpuPowerGateTogglePartitionIds[which_core]);

        /* Start the core. */
        StartCpu(which_core);

        return SmcResult::PsciSuccess;
    }

    SmcResult SmcSuspendCpu(const SmcArguments &args) {
        /* TODO */
        return SmcResult::NotImplemented;
    }

    bool IsChargerHiZModeEnabled() {
        return g_charger_hi_z_mode_enabled;
    }

    void SetChargerHiZModeEnabled(bool en) {
        g_charger_hi_z_mode_enabled = en;
    }

}
