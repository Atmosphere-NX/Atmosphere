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

namespace ams::htclow {

    R_DEFINE_NAMESPACE_RESULT_MODULE(29);

    R_DEFINE_ERROR_RESULT(ConnectionFailure,          1);
    R_DEFINE_ERROR_RESULT(UnknownDriverType,          3);
    R_DEFINE_ERROR_RESULT(NonBlockingReceiveFailed,   5);
    R_DEFINE_ERROR_RESULT(ChannelAlreadyExist,        9);
    R_DEFINE_ERROR_RESULT(ChannelNotExist,           10);

    R_DEFINE_ERROR_RESULT(OutOfResource,            151);

    R_DEFINE_ERROR_RESULT(InvalidChannelState,             200);
    R_DEFINE_ERROR_RESULT(InvalidChannelStateDisconnected, 201);

    R_DEFINE_ERROR_RANGE(InternalError, 1000, 2999);
        R_DEFINE_ERROR_RESULT(Overflow,        1001);
        R_DEFINE_ERROR_RESULT(OutOfMemory,     1002);
        R_DEFINE_ERROR_RESULT(InvalidArgument, 1003);
        R_DEFINE_ERROR_RESULT(ProtocolError,   1004);
        R_DEFINE_ERROR_RESULT(Cancelled,       1005);

        R_DEFINE_ERROR_RANGE(MuxError, 1100, 1199);
            R_DEFINE_ERROR_RESULT(ChannelBufferOverflow,         1101);
            R_DEFINE_ERROR_RESULT(ChannelBufferHasNotEnoughData, 1102);
            R_DEFINE_ERROR_RESULT(ChannelVersionNotMatched,      1103);
            R_DEFINE_ERROR_RESULT(ChannelStateTransitionError,   1104);
            R_DEFINE_ERROR_RESULT(ChannelReceiveBufferEmpty,     1106);
            R_DEFINE_ERROR_RESULT(ChannelSequenceIdNotMatched,   1107);
            R_DEFINE_ERROR_RESULT(ChannelCannotDiscard,          1108);

        R_DEFINE_ERROR_RANGE(DriverError, 1200, 1999);
            R_DEFINE_ERROR_RESULT(DriverOpened, 1201);

            R_DEFINE_ERROR_RANGE(UsbDriverError, 1400, 1499);
                R_DEFINE_ERROR_RESULT(UsbDriverUnknownError, 1401);
                R_DEFINE_ERROR_RESULT(UsbDriverBusyError,    1402);
                R_DEFINE_ERROR_RESULT(UsbDriverReceiveError, 1403);
                R_DEFINE_ERROR_RESULT(UsbDriverSendError,    1404);

        R_DEFINE_ERROR_RESULT(HtcctrlError, 2000); /* TODO: Range? */
            R_DEFINE_ERROR_RESULT(HtcctrlStateTransitionNotAllowed, 2001);
            R_DEFINE_ERROR_RESULT(HtcctrlReceiveUnexpectedPacket,   2002);

}
