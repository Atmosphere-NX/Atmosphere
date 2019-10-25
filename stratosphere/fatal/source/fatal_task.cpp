/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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
#include "fatal_task.hpp"

#include "fatal_task_error_report.hpp"
#include "fatal_task_screen.hpp"
#include "fatal_task_sound.hpp"
#include "fatal_task_clock.hpp"
#include "fatal_task_power.hpp"

namespace ams::fatal::srv {

    namespace {

        class TaskThread {
            NON_COPYABLE(TaskThread);
            private:
                static constexpr int TaskThreadPriority = 15;
            private:
                os::Thread thread;
            private:
                static void RunTaskImpl(void *arg) {
                    ITask *task = reinterpret_cast<ITask *>(arg);

                    if (R_FAILED(task->Run())) {
                        /* TODO: Log task failure, somehow? */
                    }
                }
            public:
                TaskThread() { /* ... */ }
                void StartTask(ITask *task) {
                    R_ASSERT(this->thread.Initialize(&RunTaskImpl, task, task->GetStack(), task->GetStackSize(), TaskThreadPriority));
                    R_ASSERT(this->thread.Start());
                }
        };

        class TaskManager {
            NON_COPYABLE(TaskManager);
            private:
                static constexpr size_t MaxTasks = 8;
            private:
                TaskThread task_threads[MaxTasks];
                size_t task_count = 0;
            public:
                TaskManager() { /* ... */ }
                void StartTask(ITask *task) {
                    AMS_ASSERT(this->task_count < MaxTasks);
                    this->task_threads[this->task_count++].StartTask(task);
                }
        };

        /* Global task manager. */
        TaskManager g_task_manager;

    }

    void RunTasks(const ThrowContext *ctx) {
        g_task_manager.StartTask(GetErrorReportTask(ctx));
        g_task_manager.StartTask(GetPowerControlTask(ctx));
        g_task_manager.StartTask(GetShowFatalTask(ctx));
        g_task_manager.StartTask(GetStopSoundTask(ctx));
        g_task_manager.StartTask(GetBacklightControlTask(ctx));
        g_task_manager.StartTask(GetAdjustClockTask(ctx));
        g_task_manager.StartTask(GetPowerButtonObserveTask(ctx));
        g_task_manager.StartTask(GetStateTransitionStopTask(ctx));
    }

}
