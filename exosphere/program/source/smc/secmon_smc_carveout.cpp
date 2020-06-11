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
#include "../secmon_error.hpp"
#include "../secmon_setup.hpp"
#include "secmon_smc_carveout.hpp"

namespace ams::secmon::smc {

    SmcResult SmcSetKernelCarveoutRegion(SmcArguments &args) {
        /* Decode arguments. */
        const int       index   = args.r[1];
        const uintptr_t address = args.r[2];
        const size_t    size    = args.r[3];

        /* Validate arguments. */
        SMC_R_UNLESS(0 <= index && index < KernelCarveoutCount, InvalidArgument);
        SMC_R_UNLESS(util::IsAligned(address, 128_KB),          InvalidArgument);
        SMC_R_UNLESS(util::IsAligned(size,    128_KB),          InvalidArgument);
        SMC_R_UNLESS(size <= CarveoutSizeMax,                   InvalidArgument);

        /* Set the carveout. */
        SetKernelCarveoutRegion(index, address, size);

        return SmcResult::Success;
    }

}
