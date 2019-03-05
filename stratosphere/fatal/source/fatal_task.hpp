/*
 * Copyright (c) 2018 Atmosphère-NX
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
#include <switch.h>
#include <stratosphere.hpp>
#include "fatal_types.hpp"

class IFatalTask {
    protected:
        FatalThrowContext *ctx;
        u64 title_id;
    public:
        IFatalTask(FatalThrowContext *ctx, u64 tid) : ctx(ctx), title_id(tid) { }
        virtual Result Run() = 0;
        virtual const char *GetName() const = 0;
};

void RunFatalTasks(FatalThrowContext *ctx, u64 title_id, bool error_report, Event *erpt_event, Event *battery_event);
