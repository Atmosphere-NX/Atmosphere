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
 
#include <switch.h>
#include "fatal_types.hpp"
#include "fatal_task.hpp"

#include "fatal_task_error_report.hpp"
#include "fatal_task_power.hpp"


static constexpr size_t MaxTasks = 8;
static HosThread g_task_threads[MaxTasks];
static size_t g_num_threads = 0;


static void RunTaskThreadFunc(void *arg) {
    IFatalTask *task = reinterpret_cast<IFatalTask *>(arg);
    
    Result rc = task->Run();
    if (R_FAILED(rc)) {
        /* TODO: Log task failure, somehow? */
    }
    
    /* Finish. */
    svcExitThread();
}

static void RunTask(IFatalTask *task) {    
    if (g_num_threads >= MaxTasks) {
        std::abort();
    }
    
    HosThread *cur_thread = &g_task_threads[g_num_threads++];
    
    cur_thread->Initialize(&RunTaskThreadFunc, task, 0x4000, 15);
    cur_thread->Start();
}

void RunFatalTasks(FatalContext *ctx, u64 title_id, bool error_report, Event *erpt_event, Event *battery_event) {
    RunTask(new ErrorReportTask(ctx, title_id, error_report, erpt_event));
    RunTask(new PowerControlTask(ctx, title_id, erpt_event, battery_event));
    /* TODO: RunTask(new ShowFatalTask(ctx, title_id, battery_event)); */
    /* TODO: RunTask(new StopSoundTask(ctx, title_id)); */
    /* TODO: RunTask(new BacklightControlTask(ctx, title_id)); */
    /* TODO: RunTask(new AdjustClockTask(ctx, title_id)); */
    RunTask(new PowerButtonObserveTask(ctx, title_id, erpt_event));
    RunTask(new StateTransitionStopTask(ctx, title_id));
}
