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
#include "fatal_task.hpp"

class ErrorReportTask : public IFatalTask {
    private:
        bool create_report;
        Event *erpt_event;
    private:
        void EnsureReportDirectories();
        bool GetCurrentTime(u64 *out);
        void SaveReportToSdCard();
    public:
        ErrorReportTask(FatalThrowContext *ctx, u64 title_id, bool error_report, Event *evt) : IFatalTask(ctx, title_id), create_report(error_report), erpt_event(evt) { }
        virtual Result Run() override;
        virtual const char *GetName() const override {
            return "WriteErrorReport";
        }
};