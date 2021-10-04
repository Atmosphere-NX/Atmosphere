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

namespace ams::usb {

    R_DEFINE_NAMESPACE_RESULT_MODULE(140);

    R_DEFINE_ERROR_RESULT(NotInitialized,       0);
    R_DEFINE_ERROR_RESULT(AlreadyInitialized,   1);

    R_DEFINE_ERROR_RANGE(InvalidParameter, 100, 199);
        R_DEFINE_ERROR_RESULT(AlignmentError, 103);

    R_DEFINE_ERROR_RESULT(OperationDenied,    201);
    R_DEFINE_ERROR_RESULT(MemAllocFailure,    202);
    R_DEFINE_ERROR_RESULT(ResourceBusy,       206);
    R_DEFINE_ERROR_RESULT(InternalStateError, 207);

    R_DEFINE_ERROR_RESULT(TransactionError,   401);
    R_DEFINE_ERROR_RESULT(Interrupted,        409);

}
