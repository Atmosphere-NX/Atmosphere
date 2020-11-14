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
#if defined(ATMOSPHERE_IS_STRATOSPHERE)
#include <stratosphere.hpp>
#elif defined(ATMOSPHERE_IS_MESOSPHERE)
#include <mesosphere.hpp>
#elif defined(ATMOSPHERE_IS_EXOSPHERE)
#include <exosphere.hpp>
#else
#include <vapours.hpp>
#endif
#include "sdmmc_clock_reset_controller.hpp"
#include "sdmmc_clock_reset_controller.reg.board.nintendo_nx.hpp"
#include "sdmmc_clock_reset_controller.pcv.board.nintendo_nx.hpp"

namespace ams::sdmmc::impl::ClockResetController {

    namespace {

        constinit bool g_is_module_initialized[Module_Count] = {};

        #if defined(AMS_SDMMC_THREAD_SAFE)

            constinit os::SdkMutex g_module_mutex;

            #define AMS_SDMMC_LOCK_MODULE_MUTEX() std::scoped_lock lk(g_module_mutex)

        #else

            #define AMS_SDMMC_LOCK_MODULE_MUTEX()

        #endif

        #if defined(AMS_SDMMC_USE_PCV_CLOCK_RESET_CONTROL)
            constinit bool g_is_pcv_control = false;

            #define AMS_SDMMC_IF_IS_PCV_CONTROL() if (g_is_pcv_control)

        #else

            #define AMS_SDMMC_IF_IS_PCV_CONTROL() if constexpr (false)

        #endif

    }

    void Initialize(Module module) {
        /* Acquire exclusive access to module state. */
        AMS_SDMMC_LOCK_MODULE_MUTEX();

        /* Mark the module as initialized. */
        g_is_module_initialized[module] = true;

        /* Initialize the module. */
        AMS_SDMMC_IF_IS_PCV_CONTROL() {
            ClockResetController::pcv::Initialize(module);
        } else {
            ClockResetController::reg::Initialize(module);
        }
    }

    void Finalize(Module module) {
        /* Acquire exclusive access to module state. */
        AMS_SDMMC_LOCK_MODULE_MUTEX();

        /* Finalize the module. */
        AMS_SDMMC_IF_IS_PCV_CONTROL() {
            ClockResetController::pcv::Finalize(module);
        } else {
            ClockResetController::reg::Finalize(module);
        }

        /* Mark the module as finalized. */
        g_is_module_initialized[module] = false;
    }

    bool IsAvailable(Module module) {
        AMS_SDMMC_IF_IS_PCV_CONTROL() {
            return ClockResetController::pcv::IsAvailable(module);
        } else {
            return ClockResetController::reg::IsAvailable(module);
        }
    }

    #if defined(AMS_SDMMC_USE_PCV_CLOCK_RESET_CONTROL)
    void SwitchToPcvControl() {
        /* Acquire exclusive access to module state. */
        AMS_SDMMC_LOCK_MODULE_MUTEX();

        /* If we're already using pcv control, we don't need to do anything. */
        AMS_SDMMC_IF_IS_PCV_CONTROL() {
            return;
        }

        /* Finalize all modules. */
        for (int i = 0; i < Module_Count; ++i) {
            if (g_is_module_initialized[i]) {
                ClockResetController::reg::Finalize(static_cast<Module>(i));
            }
        }

        /* Mark that we've switched to pcv control. */

        /* Initialize modules using pcv control. */
        for (int i = 0; i < Module_Count; ++i) {
            if (g_is_module_initialized[i]) {
                ClockResetController::pcv::Initialize(static_cast<Module>(i));
            }
        }
    }
    #endif

    void SetClockFrequencyKHz(u32 *out_actual_frequency, Module module, u32 target_frequency) {
        AMS_SDMMC_IF_IS_PCV_CONTROL() {
            return ClockResetController::pcv::SetClockFrequencyKHz(out_actual_frequency, module, target_frequency);
        } else {
            return ClockResetController::reg::SetClockFrequencyKHz(out_actual_frequency, module, target_frequency);
        }
    }

    void AssertReset(Module module) {
        AMS_SDMMC_IF_IS_PCV_CONTROL() {
            return ClockResetController::pcv::AssertReset(module);
        } else {
            return ClockResetController::reg::AssertReset(module);
        }
    }

    void ReleaseReset(Module module, u32 target_frequency_khz) {
        AMS_SDMMC_IF_IS_PCV_CONTROL() {
            return ClockResetController::pcv::ReleaseReset(module, target_frequency_khz);
        } else {
            return ClockResetController::reg::ReleaseReset(module, target_frequency_khz);
        }
    }


}
