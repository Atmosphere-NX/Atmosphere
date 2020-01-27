/*
 * Copyright (c) 2019 Atmosphère-NX
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

#include "exceptions.h"

typedef enum DebugEventType {
    DBGEVENT_DEBUGGER_BREAK = 0,
    DBGEVENT_EXCEPTION,
    DBGEVENT_CORE_ON,
    DBGEVENT_CORE_OFF,
    DBGEVENT_EXIT,
    DBGEVENT_OUTPUT_STRING,
} DebugEventType;

typedef struct OutputStringDebugEventInfo {
    uintptr_t address;
    size_t size;
} OutputStringDebugEventInfo;

typedef struct DebugEventInfo {
    DebugEventType type;
    u32 coreId;
    ExceptionStackFrame *frame;
    union {
        OutputStringDebugEventInfo outputString;
    };
} DebugEventInfo;

void debugManagerPauseSgiHandler(void);

// Hypervisor interrupts will be serviced during the pause-wait
void debugManagerHandlePause(void);

// Note: these functions are not reentrant EXCEPT debugPauseCores(1 << currentCoreId)
// "Pause" makes sure all cores reaches the pause function before proceeding.
// "Unpause" doesn't synchronize, it just makes sure the core resumes & that "pause" can be called again.
void debugManagerPauseCores(u32 coreList);
void debugManagerUnpauseCores(u32 coreList, u32 singleStepList);
