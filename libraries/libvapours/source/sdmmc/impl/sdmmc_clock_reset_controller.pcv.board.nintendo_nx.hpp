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
#pragma once
#include <vapours.hpp>
#include "sdmmc_clock_reset_controller.hpp"

namespace ams::sdmmc::impl::ClockResetController::pcv {

    void Initialize(Module module);
    void Finalize(Module module);
    bool IsAvailable(Module module);

    void SetClockFrequencyKHz(u32 *out_actual_frequency, Module module, u32 target_frequency);
    void AssertReset(Module module);
    void ReleaseReset(Module module, u32 target_frequency_khz);

}
