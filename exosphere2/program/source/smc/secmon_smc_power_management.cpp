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
#include "../secmon_cache.hpp"
#include "../secmon_cpu_context.hpp"
#include "../secmon_error.hpp"
#include "secmon_smc_power_management.hpp"
#include "secmon_smc_se_lock.hpp"

namespace ams::secmon {

    /* Declare assembly functionality. */
    void *GetCoreExceptionStackVirtual();

}

namespace ams::secmon::smc {

    /* Declare assembly power-management functionality. */
    void PivotStackAndInvoke(void *stack, void (*function)());
    void FinalizePowerOff();

    namespace {

        constexpr inline uintptr_t PMC     = MemoryRegionVirtualDevicePmc.GetAddress();
        constexpr inline uintptr_t GPIO    = MemoryRegionVirtualDeviceGpio.GetAddress();
        constexpr inline uintptr_t CLK_RST = MemoryRegionVirtualDeviceClkRst.GetAddress();

        constexpr inline uintptr_t CommonSmcStackTop = MemoryRegionVirtualTzramVolatileData.GetEndAddress() - (0x80 * (NumCores - 1));

        enum PowerStateType {
            PowerStateType_StandBy   = 0,
            PowerStateType_PowerDown = 1,
        };

        enum PowerStateId {
            PowerStateId_Sc7 = 27,
        };

        /* http://infocenter.arm.com/help/topic/com.arm.doc.den0022d/Power_State_Coordination_Interface_PDD_v1_1_DEN0022D.pdf Page 46 */
        struct SuspendCpuPowerState {
            using StateId    = util::BitPack32::Field< 0, 16, PowerStateId>;
            using StateType  = util::BitPack32::Field<16,  1, PowerStateType>;
            using PowerLevel = util::BitPack32::Field<24,  2, u32>;
        };

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

        void PowerOffCpu() {
            /* Get the current core id. */
            const auto core_id = hw::GetCurrentCoreId();

            /* Configure the flow controller to prepare for shutting down the current core. */
            flow::SetCpuCsr(core_id, FLOW_CTLR_CPUN_CSR_ENABLE_EXT_POWERGATE_CPU_ONLY);
            flow::SetHaltCpuEvents(core_id, false);
            flow::SetCc4Ctrl(core_id, 0);

            /* Save the core's context for restoration on next power-on. */
            SaveDebugRegisters();
            SetCoreOff();

            /* Ensure there are no pending memory transactions prior to our power-down. */
            FlushEntireDataCache();

            /* Finalize our powerdown and wait for an interrupt. */
            FinalizePowerOff();
        }

        void ValidateSocStateForSuspend() {
            /* TODO */
        }

        void SaveSecureContextAndSuspend() {
            /* TODO */

            /* Finalize our powerdown and wait for an interrupt. */
            FinalizePowerOff();
        }

        SmcResult SuspendCpuImpl(SmcArguments &args) {
            /* Decode arguments. */
            const util::BitPack32 power_state = { static_cast<u32>(args.r[1]) };
            const uintptr_t       entry_point = args.r[2];
            const uintptr_t       context_id  = args.r[3];

            const auto state_type = power_state.Get<SuspendCpuPowerState::StateType>();
            const auto state_id   = power_state.Get<SuspendCpuPowerState::StateId>();

            const auto core_id = hw::GetCurrentCoreId();

            /* Validate arguments. */
            SMC_R_UNLESS(state_type == PowerStateType_PowerDown, PsciDenied);
            SMC_R_UNLESS(state_id   == PowerStateId_Sc7,         PsciDenied);

            /* Orchestrate charger transition to Hi-Z mode if needed. */
            if (IsChargerHiZModeEnabled()) {
                /* Ensure we can do comms over i2c-1. */
                clkrst::EnableI2c1Clock();

                /* If the charger isn't in hi-z mode, perform a transition. */
                if (!charger::IsHiZMode()) {
                    charger::EnterHiZMode();

                    /* Wait up to 50ms for the transition to complete. */
                    const auto start_time = util::GetMicroSeconds();
                    auto current_time = start_time;
                    while ((current_time - start_time) <= 50'000) {
                        if (auto intr_status = reg::Read(GPIO + 0x634); (intr_status & 1) == 0) {
                            /* Wait 256 us to ensure the transition completes. */
                            util::WaitMicroSeconds(256);
                            break;
                        }
                        current_time = util::GetMicroSeconds();
                    }
                }

                /* Disable i2c-1, since we're done communicating over it. */
                clkrst::DisableI2c1Clock();
            }

            /* Enable wake event detection. */
            pmc::EnableWakeEventDetection();

            /* Ensure that i2c-5 is usable for communicating with the pmic. */
            clkrst::EnableI2c5Clock();
            i2c::Initialize(i2c::Port_5);

            /* Orchestrate sleep entry with the pmic. */
            pmic::EnableSleep();

            /* Ensure that the soc is in a state valid for us to suspend. */
            ValidateSocStateForSuspend();

            /* Configure the pmc for sc7 entry. */
            pmc::ConfigureForSc7Entry();

            /* Configure the flow controller for sc7 entry. */
            flow::SetCc4Ctrl(core_id, 0);
            flow::SetHaltCpuEvents(core_id, false);
            flow::ClearL2FlushControl();
            flow::SetCpuCsr(core_id, FLOW_CTLR_CPUN_CSR_ENABLE_EXT_POWERGATE_CPU_TURNOFF_CPURAIL);

            /* Save the entry context. */
            SetEntryContext(core_id, entry_point, context_id);

            /* Configure the cpu context for reset. */
            SaveDebugRegisters();
            SetCoreOff();
            SetResetExpected(true);

            /* Switch to use the common smc stack (all other cores are off), and perform suspension. */
            PivotStackAndInvoke(reinterpret_cast<void *>(CommonSmcStackTop), SaveSecureContextAndSuspend);

            /* This code will never be reached. */
            __builtin_unreachable();
        }

    }

    SmcResult SmcPowerOffCpu(SmcArguments &args) {
        /* Get the current core id. */
        const auto core_id = hw::GetCurrentCoreId();

        /* Note that we're expecting a reset for the current core. */
        SetResetExpected(true);

        /* If we're on the final core, shut down directly. Otherwise, invoke with special stack. */
        if (core_id == NumCores - 1) {
            PowerOffCpu();
        } else {
            PivotStackAndInvoke(GetCoreExceptionStackVirtual(), PowerOffCpu);
        }

        /* This code will never be reached. */
        __builtin_unreachable();
    }

    SmcResult SmcPowerOnCpu(SmcArguments &args) {
        /* Get and validate the core to power on. */
        const int which_core = args.r[1];
        SMC_R_UNLESS(0 <= which_core && which_core < NumCores, PsciInvalidParameters);

        /* Ensure the core isn't already on. */
        SMC_R_UNLESS(!IsCoreOn(which_core), PsciAlreadyOn);

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

    SmcResult SmcSuspendCpu(SmcArguments &args) {
        return LockSecurityEngineAndInvoke(args, SuspendCpuImpl);
    }

    bool IsChargerHiZModeEnabled() {
        return g_charger_hi_z_mode_enabled;
    }

    void SetChargerHiZModeEnabled(bool en) {
        g_charger_hi_z_mode_enabled = en;
    }

}
