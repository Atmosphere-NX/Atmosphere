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
#include <vapours.hpp>

namespace ams::kern::svc {

    /*   7 */ using ::ams::svc::ResultOutOfSessions;

    /*  14 */ using ::ams::svc::ResultInvalidArgument;

    /*  33 */ using ::ams::svc::ResultNotImplemented;

    /*  54 */ using ::ams::svc::ResultStopProcessingException;

    /*  57 */ using ::ams::svc::ResultNoSynchronizationObject;

    /*  59 */ using ::ams::svc::ResultTerminationRequested;

    /*  70 */ using ::ams::svc::ResultNoEvent;

    /* 101 */ using ::ams::svc::ResultInvalidSize;
    /* 102 */ using ::ams::svc::ResultInvalidAddress;
    /* 103 */ using ::ams::svc::ResultOutOfResource;
    /* 104 */ using ::ams::svc::ResultOutOfMemory;
    /* 105 */ using ::ams::svc::ResultOutOfHandles;
    /* 106 */ using ::ams::svc::ResultInvalidCurrentMemory;

    /* 108 */ using ::ams::svc::ResultInvalidNewMemoryPermission;

    /* 110 */ using ::ams::svc::ResultInvalidMemoryRegion;

    /* 112 */ using ::ams::svc::ResultInvalidPriority;
    /* 113 */ using ::ams::svc::ResultInvalidCoreId;
    /* 114 */ using ::ams::svc::ResultInvalidHandle;
    /* 115 */ using ::ams::svc::ResultInvalidPointer;
    /* 116 */ using ::ams::svc::ResultInvalidCombination;
    /* 117 */ using ::ams::svc::ResultTimedOut;
    /* 118 */ using ::ams::svc::ResultCancelled;
    /* 119 */ using ::ams::svc::ResultOutOfRange;
    /* 120 */ using ::ams::svc::ResultInvalidEnumValue;
    /* 121 */ using ::ams::svc::ResultNotFound;
    /* 122 */ using ::ams::svc::ResultBusy;
    /* 123 */ using ::ams::svc::ResultSessionClosed;
    /* 124 */ using ::ams::svc::ResultNotHandled;
    /* 125 */ using ::ams::svc::ResultInvalidState;
    /* 126 */ using ::ams::svc::ResultReservedUsed;
    /* 127 */ using ::ams::svc::ResultNotSupported;
    /* 128 */ using ::ams::svc::ResultDebug;
    /* 129 */ using ::ams::svc::ResultNoThread;
    /* 130 */ using ::ams::svc::ResultUnknownThread;
    /* 131 */ using ::ams::svc::ResultPortClosed;
    /* 132 */ using ::ams::svc::ResultLimitReached;
    /* 133 */ using ::ams::svc::ResultInvalidMemoryPool;

    /* 258 */ using ::ams::svc::ResultReceiveListBroken;
    /* 259 */ using ::ams::svc::ResultOutOfAddressSpace;
    /* 260 */ using ::ams::svc::ResultMessageTooLarge;

    /* 517 */ using ::ams::svc::ResultInvalidProcessId;
    /* 518 */ using ::ams::svc::ResultInvalidThreadId;
    /* 519 */ using ::ams::svc::ResultInvalidId;
    /* 520 */ using ::ams::svc::ResultProcessTerminated;

}
