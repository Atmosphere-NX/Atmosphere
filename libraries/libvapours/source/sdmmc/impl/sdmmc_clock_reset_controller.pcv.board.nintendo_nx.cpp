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
#include "sdmmc_clock_reset_controller.pcv.board.nintendo_nx.hpp"

namespace ams::sdmmc::impl::ClockResetController::pcv {

    #if defined(AMS_SDMMC_USE_PCV_CLOCK_RESET_CONTROL)

        void Initialize(Module module) {
            AMS_UNUSED(module);
            AMS_ABORT("PCV Control not implemented");
        }

        void Finalize(Module module) {
            AMS_UNUSED(module);
            AMS_ABORT("PCV Control not implemented");
        }

        bool IsAvailable(Module module) {
            AMS_UNUSED(module);
            AMS_ABORT("PCV Control not implemented");
        }

        void SetClockFrequencyKHz(u32 *out_actual_frequency, Module module, u32 target_frequency) {
            AMS_UNUSED(out_actual_frequency, module, target_frequency);
            AMS_ABORT("PCV Control not implemented");
        }

        void AssertReset(Module module) {
            AMS_UNUSED(module);
            AMS_ABORT("PCV Control not implemented");
        }

        void ReleaseReset(Module module, u32 target_frequency_khz) {
            AMS_UNUSED(module, target_frequency_khz);
            AMS_ABORT("PCV Control not implemented");
        }

    #else

        void Initialize(Module module) {
            AMS_UNUSED(module);
            AMS_ABORT("PCV Control not supported");
        }

        void Finalize(Module module) {
            AMS_UNUSED(module);
            AMS_ABORT("PCV Control not supported");
        }

        bool IsAvailable(Module module) {
            AMS_UNUSED(module);
            AMS_ABORT("PCV Control not supported");
        }

        void SetClockFrequencyKHz(u32 *out_actual_frequency, Module module, u32 target_frequency) {
            AMS_UNUSED(out_actual_frequency, module, target_frequency);
            AMS_ABORT("PCV Control not supported");
        }

        void AssertReset(Module module) {
            AMS_UNUSED(module);
            AMS_ABORT("PCV Control not supported");
        }

        void ReleaseReset(Module module, u32 target_frequency_khz) {
            AMS_UNUSED(module, target_frequency_khz);
            AMS_ABORT("PCV Control not supported");
        }

    #endif

}
