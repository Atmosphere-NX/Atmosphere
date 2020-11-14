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

namespace ams::svc {

    R_DEFINE_NAMESPACE_RESULT_MODULE(1);

    R_DEFINE_ERROR_RESULT(OutOfSessions,                7);

    R_DEFINE_ERROR_RESULT(InvalidArgument,              14);

    R_DEFINE_ERROR_RESULT(NotImplemented,               33);

    R_DEFINE_ERROR_RESULT(StopProcessingException,      54);

    R_DEFINE_ERROR_RESULT(NoSynchronizationObject,      57);

    R_DEFINE_ERROR_RESULT(TerminationRequested,         59);

    R_DEFINE_ERROR_RESULT(NoEvent,                      70);

    R_DEFINE_ERROR_RESULT(InvalidSize,                  101);
    R_DEFINE_ERROR_RESULT(InvalidAddress,               102);
    R_DEFINE_ERROR_RESULT(OutOfResource,                103);
    R_DEFINE_ERROR_RESULT(OutOfMemory,                  104);
    R_DEFINE_ERROR_RESULT(OutOfHandles,                 105);
    R_DEFINE_ERROR_RESULT(InvalidCurrentMemory,         106);

    R_DEFINE_ERROR_RESULT(InvalidNewMemoryPermission,   108);

    R_DEFINE_ERROR_RESULT(InvalidMemoryRegion,          110);

    R_DEFINE_ERROR_RESULT(InvalidPriority,              112);
    R_DEFINE_ERROR_RESULT(InvalidCoreId,                113);
    R_DEFINE_ERROR_RESULT(InvalidHandle,                114);
    R_DEFINE_ERROR_RESULT(InvalidPointer,               115);
    R_DEFINE_ERROR_RESULT(InvalidCombination,           116);
    R_DEFINE_ERROR_RESULT(TimedOut,                     117);
    R_DEFINE_ERROR_RESULT(Cancelled,                    118);
    R_DEFINE_ERROR_RESULT(OutOfRange,                   119);
    R_DEFINE_ERROR_RESULT(InvalidEnumValue,             120);
    R_DEFINE_ERROR_RESULT(NotFound,                     121);
    R_DEFINE_ERROR_RESULT(Busy,                         122);
    R_DEFINE_ERROR_RESULT(SessionClosed,                123);
    R_DEFINE_ERROR_RESULT(NotHandled,                   124);
    R_DEFINE_ERROR_RESULT(InvalidState,                 125);
    R_DEFINE_ERROR_RESULT(ReservedUsed,                 126);
    R_DEFINE_ERROR_RESULT(NotSupported,                 127);
    R_DEFINE_ERROR_RESULT(Debug,                        128);
    R_DEFINE_ERROR_RESULT(NoThread,                     129);
    R_DEFINE_ERROR_RESULT(UnknownThread,                130);
    R_DEFINE_ERROR_RESULT(PortClosed,                   131);
    R_DEFINE_ERROR_RESULT(LimitReached,                 132);
    R_DEFINE_ERROR_RESULT(InvalidMemoryPool,            133);

    R_DEFINE_ERROR_RESULT(ReceiveListBroken,            258);
    R_DEFINE_ERROR_RESULT(OutOfAddressSpace,            259);
    R_DEFINE_ERROR_RESULT(MessageTooLarge,              260);

    R_DEFINE_ERROR_RESULT(InvalidProcessId,             517);
    R_DEFINE_ERROR_RESULT(InvalidThreadId,              518);
    R_DEFINE_ERROR_RESULT(InvalidId,                    519);
    R_DEFINE_ERROR_RESULT(ProcessTerminated,            520);

}
