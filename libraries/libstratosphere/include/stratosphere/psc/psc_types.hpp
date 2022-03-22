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

#pragma once
#include <vapours.hpp>

namespace ams::psc {

    enum PmState {
        PmState_FullAwake                   = 0,
        PmState_MinimumAwake                = 1,
        PmState_SleepReady                  = 2,
        PmState_EssentialServicesSleepReady = 3,
        PmState_EssentialServicesAwake      = 4,
        PmState_ShutdownReady               = 5,
        PmState_Unknown                     = 6,
    };

    constexpr inline u32 MaximumDependencyLevels  = 20;

    struct PmFlag {

    };

    using PmFlagSet = util::BitFlagSet<BITSIZEOF(u32), PmFlag>;

}
