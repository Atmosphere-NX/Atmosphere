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

namespace ams::spl {

    R_DEFINE_NAMESPACE_RESULT_MODULE(26);

    R_DEFINE_ERROR_RANGE(SecureMonitorError, 0, 99);
        R_DEFINE_ERROR_RESULT(SecureMonitorNotImplemented,        1);
        R_DEFINE_ERROR_RESULT(SecureMonitorInvalidArgument,       2);
        R_DEFINE_ERROR_RESULT(SecureMonitorBusy,                  3);
        R_DEFINE_ERROR_RESULT(SecureMonitorNoAsyncOperation,      4);
        R_DEFINE_ERROR_RESULT(SecureMonitorInvalidAsyncOperation, 5);
        R_DEFINE_ERROR_RESULT(SecureMonitorNotPermitted,          6);
        R_DEFINE_ERROR_RESULT(SecureMonitorNotInitialized,        7);

    R_DEFINE_ERROR_RESULT(InvalidSize,                  100);
    R_DEFINE_ERROR_RESULT(UnknownSecureMonitorError,    101);
    R_DEFINE_ERROR_RESULT(DecryptionFailed,             102);

    R_DEFINE_ERROR_RESULT(OutOfKeySlots,                104);
    R_DEFINE_ERROR_RESULT(InvalidKeySlot,               105);
    R_DEFINE_ERROR_RESULT(BootReasonAlreadySet,         106);
    R_DEFINE_ERROR_RESULT(BootReasonNotSet,             107);
    R_DEFINE_ERROR_RESULT(InvalidArgument,              108);

}
