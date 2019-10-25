/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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
#include "results_common.hpp"

namespace ams::fatal {

    R_DEFINE_NAMESPACE_RESULT_MODULE(163);

    R_DEFINE_ERROR_RESULT(AllocationFailed,                    1);
    R_DEFINE_ERROR_RESULT(NullGraphicsBuffer,                  2);
    R_DEFINE_ERROR_RESULT(AlreadyThrown,                       3);
    R_DEFINE_ERROR_RESULT(TooManyEvents,                       4);
    R_DEFINE_ERROR_RESULT(InRepairWithoutVolHeld,              5);
    R_DEFINE_ERROR_RESULT(InRepairWithoutTimeReviserCartridge, 6);

}
