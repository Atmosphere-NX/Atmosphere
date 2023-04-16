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

#include <vapours/results.hpp>

/* NOTE: These results are all custom, and not official. */
R_DEFINE_NAMESPACE_RESULT_MODULE(haze, 420);

namespace haze {

    R_DEFINE_ERROR_RESULT(RegistrationFailed,     1);
    R_DEFINE_ERROR_RESULT(NotConfigured,          2);
    R_DEFINE_ERROR_RESULT(TransferFailed,         3);
    R_DEFINE_ERROR_RESULT(StopRequested,          4);
    R_DEFINE_ERROR_RESULT(EndOfTransmission,      5);
    R_DEFINE_ERROR_RESULT(UnknownPacketType,      6);
    R_DEFINE_ERROR_RESULT(SessionNotOpen,         7);
    R_DEFINE_ERROR_RESULT(OutOfMemory,            8);
    R_DEFINE_ERROR_RESULT(ObjectNotFound,         9);
    R_DEFINE_ERROR_RESULT(StorageNotFound,       10);
    R_DEFINE_ERROR_RESULT(OperationNotSupported, 11);
    R_DEFINE_ERROR_RESULT(UnknownRequestType,    12);
    R_DEFINE_ERROR_RESULT(UnknownPropertyCode,   13);
    R_DEFINE_ERROR_RESULT(GeneralFailure,        14);

}
