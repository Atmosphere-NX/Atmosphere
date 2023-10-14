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
    R_DEFINE_ERROR_RESULT(FocusLost,              5);
    R_DEFINE_ERROR_RESULT(EndOfTransmission,      6);
    R_DEFINE_ERROR_RESULT(UnknownPacketType,      7);
    R_DEFINE_ERROR_RESULT(SessionNotOpen,         8);
    R_DEFINE_ERROR_RESULT(OutOfMemory,            9);
    R_DEFINE_ERROR_RESULT(InvalidObjectId,       10);
    R_DEFINE_ERROR_RESULT(InvalidStorageId,      11);
    R_DEFINE_ERROR_RESULT(OperationNotSupported, 12);
    R_DEFINE_ERROR_RESULT(UnknownRequestType,    13);
    R_DEFINE_ERROR_RESULT(UnknownPropertyCode,   14);
    R_DEFINE_ERROR_RESULT(InvalidPropertyValue,  15);
    R_DEFINE_ERROR_RESULT(InvalidArgument,       16);
    R_DEFINE_ERROR_RESULT(GroupSpecified,        17);
    R_DEFINE_ERROR_RESULT(DepthSpecified,        18);

}
