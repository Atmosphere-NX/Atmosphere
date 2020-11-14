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
#include <vapours/results/results_common.hpp>

namespace ams::os {

    R_DEFINE_NAMESPACE_RESULT_MODULE(3);

    R_DEFINE_ERROR_RESULT(Busy,                         4);

    R_DEFINE_ERROR_RESULT(OutOfMemory,                  8);
    R_DEFINE_ERROR_RESULT(OutOfResource,                9);

    R_DEFINE_ERROR_RESULT(OutOfVirtualAddressSpace,    12);
    R_DEFINE_ERROR_RESULT(ResourceLimit,               13);

    R_DEFINE_ERROR_RESULT(OutOfHandles,               500);
    R_DEFINE_ERROR_RESULT(InvalidHandle,              501);
    R_DEFINE_ERROR_RESULT(InvalidCurrentMemoryState,  502);
    R_DEFINE_ERROR_RESULT(InvalidTransferMemoryState, 503);
    R_DEFINE_ERROR_RESULT(InvalidTransferMemorySize,  504);
    R_DEFINE_ERROR_RESULT(OutOfTransferMemory,        505);
    R_DEFINE_ERROR_RESULT(OutOfAddressSpace,          506);

}
