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
#include "secmon_smc_error.hpp"

namespace ams::secmon::smc {

    SmcResult SmcShowError(SmcArguments &args) {
        /* Decode arguments. */
        const u32 r = ((args.r[1] >> 8) & 0xF);
        const u32 g = ((args.r[1] >> 4) & 0xF);
        const u32 b = ((args.r[1] >> 0) & 0xF);

        const u32 rgb = (b << 8) | (g << 4) | (r << 0);

        /* Set the error info. */
        SetError(pkg1::MakeKernelPanicResetInfo(rgb));

        /* Reboot. */
        ErrorReboot();

        /* This point will never be reached. */
        __builtin_unreachable();
    }

}
