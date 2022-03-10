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
#include <vapours/results/results_common.hpp>

R_DEFINE_NAMESPACE_RESULT_MODULE(ams::dmnt, 13);

namespace ams::dmnt {

    R_DEFINE_ERROR_RESULT(Unknown,           1);
    R_DEFINE_ERROR_RESULT(DebuggingDisabled, 2);

    /* Atmosphere extension. */
    // namespace cheat {

        R_DEFINE_ABSTRACT_ERROR_RANGE_NS(cheat, CheatError, 6500, 6599);
            R_DEFINE_ERROR_RESULT_NS(cheat, CheatNotAttached,     6500);
            R_DEFINE_ERROR_RESULT_NS(cheat, CheatNullBuffer,      6501);
            R_DEFINE_ERROR_RESULT_NS(cheat, CheatInvalidBuffer,   6502);
            R_DEFINE_ERROR_RESULT_NS(cheat, CheatUnknownId,       6503);
            R_DEFINE_ERROR_RESULT_NS(cheat, CheatOutOfResource,   6504);
            R_DEFINE_ERROR_RESULT_NS(cheat, CheatInvalid,         6505);
            R_DEFINE_ERROR_RESULT_NS(cheat, CheatCannotDisable,   6506);

        R_DEFINE_ABSTRACT_ERROR_RANGE_NS(cheat, FrozenAddressError, 6600, 6699);
            R_DEFINE_ERROR_RESULT_NS(cheat, FrozenAddressInvalidWidth,  6600);
            R_DEFINE_ERROR_RESULT_NS(cheat, FrozenAddressAlreadyExists, 6601);
            R_DEFINE_ERROR_RESULT_NS(cheat, FrozenAddressNotFound,      6602);
            R_DEFINE_ERROR_RESULT_NS(cheat, FrozenAddressOutOfResource, 6603);

        R_DEFINE_ABSTRACT_ERROR_RANGE_NS(cheat, VirtualMachineError, 6700, 6799);
            R_DEFINE_ERROR_RESULT_NS(cheat, VirtualMachineInvalidConditionDepth, 6700);

    // }

}
