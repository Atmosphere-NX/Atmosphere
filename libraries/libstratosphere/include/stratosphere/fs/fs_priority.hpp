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

namespace ams::fs {

    enum Priority {
        Priority_Realtime = 0,
        Priority_Normal   = 1,
        Priority_Low      = 2,
    };

    enum PriorityRaw {
        PriorityRaw_Realtime   = 0,
        PriorityRaw_Normal     = 1,
        PriorityRaw_Low        = 2,
        PriorityRaw_Background = 3,
    };

    Priority GetPriorityOnCurrentThread();
    Priority GetPriority(os::ThreadType *thread);
    PriorityRaw GetPriorityRawOnCurrentThread();
    PriorityRaw GetPriorityRaw(os::ThreadType *thread);

    void SetPriorityOnCurrentThread(Priority prio);
    void SetPriority(os::ThreadType *thread, Priority prio);
    void SetPriorityRawOnCurrentThread(PriorityRaw prio);
    void SetPriorityRaw(os::ThreadType *thread, PriorityRaw prio);

}
