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
#include <stratosphere.hpp>
#include "pm_process_tracker.hpp"
#include "pm_process_info.hpp"
#include "pm_spec.hpp"

namespace ams::pm::impl {

    namespace {

        /* Global process event. */
        constinit os::SystemEventType g_process_event;

    }

    void ProcessTracker::Initialize(void *stack, size_t stack_size) {
        /* Initialize our events. */
        os::InitializeEvent(std::addressof(m_request_event), false, os::EventClearMode_AutoClear);
        os::InitializeEvent(std::addressof(m_reply_event),   false, os::EventClearMode_AutoClear);

        /* Our process count should initially be zero. */
        m_process_count = 0;

        /* Create the process tracking thread. */
        R_ABORT_UNLESS(os::CreateThread(std::addressof(m_thread), ProcessTracker::ThreadFunction, this, stack, stack_size, AMS_GET_SYSTEM_THREAD_PRIORITY(pm, ProcessTrack)));
        os::SetThreadNamePointer(std::addressof(m_thread), AMS_GET_SYSTEM_THREAD_NAME(pm, ProcessTrack));
    }

    void ProcessTracker::StartThread() {
        /* Start thread. */
        os::StartThread(std::addressof(m_thread));
    }

    void ProcessTracker::ThreadBody() {
        /* This is the main loop of the process tracking thread. */

        /* Setup multi wait/holders. */
        os::MultiWaitType process_multi_wait;
        os::MultiWaitHolderType enqueue_event_holder;
        os::InitializeMultiWait(std::addressof(process_multi_wait));
        os::InitializeMultiWaitHolder(std::addressof(enqueue_event_holder), std::addressof(m_request_event));
        os::LinkMultiWaitHolder(std::addressof(process_multi_wait), std::addressof(enqueue_event_holder));

        while (true) {
            auto signaled_holder = os::WaitAny(std::addressof(process_multi_wait));
            if (signaled_holder == std::addressof(enqueue_event_holder)) {
                /* TryWait will clear signaled, preventing duplicate notifications. */
                if (os::TryWaitEvent(std::addressof(m_request_event))) {
                    /* Link the process to our multi-wait. */
                    os::LinkMultiWaitHolder(std::addressof(process_multi_wait), m_queued_process_info->GetMultiWaitHolder());
                    m_queued_process_info = nullptr;

                    /* Increment our process count. */
                    ++m_process_count;

                    /* Reply. */
                    os::SignalEvent(std::addressof(m_reply_event));
                }
            } else {
                /* Some process was signaled. */
                this->OnProcessSignaled(reinterpret_cast<ProcessInfo *>(os::GetMultiWaitHolderUserData(signaled_holder)));
            }
        }
    }

    void ProcessTracker::QueueEntry(ProcessInfo *process_info) {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        /* Request to enqueue the process info. */
        m_queued_process_info = process_info;
        os::SignalEvent(std::addressof(m_request_event));

        /* Wait for acknowledgement. */
        os::WaitEvent(std::addressof(m_reply_event));
    }

    void ProcessTracker::OnProcessSignaled(ProcessInfo *process_info) {
        /* Get the process list. */
        auto list = GetProcessList();

        /* Reset the process's signal. */
        svc::ResetSignal(process_info->GetHandle());

        /* Update the process's state. */
        const svc::ProcessState old_state = process_info->GetState();
        {
            s64 tmp = 0;
            R_ABORT_UNLESS(svc::GetProcessInfo(std::addressof(tmp), process_info->GetHandle(), svc::ProcessInfoType_ProcessState));
            process_info->SetState(static_cast<svc::ProcessState>(tmp));
        }
        const svc::ProcessState new_state = process_info->GetState();

        /* If we're transitioning away from crashed, clear waiting attached. */
        if (old_state == svc::ProcessState_Crashed && new_state != svc::ProcessState_Crashed) {
            process_info->ClearExceptionWaitingAttach();
        }

        switch (new_state) {
            case svc::ProcessState_Created:
            case svc::ProcessState_CreatedAttached:
            case svc::ProcessState_Terminating:
                break;
            case svc::ProcessState_Running:
                if (process_info->ShouldSignalOnDebugEvent()) {
                    process_info->ClearSuspended();
                    process_info->SetSuspendedStateChanged();
                    os::SignalSystemEvent(std::addressof(g_process_event));
                } else if (hos::GetVersion() >= hos::Version_2_0_0 && process_info->ShouldSignalOnStart()) {
                    process_info->SetStartedStateChanged();
                    process_info->ClearSignalOnStart();
                    os::SignalSystemEvent(std::addressof(g_process_event));
                }
                process_info->ClearUnhandledException();
                break;
            case svc::ProcessState_Crashed:
                if (!process_info->HasUnhandledException()) {
                    process_info->SetExceptionOccurred();
                    os::SignalSystemEvent(std::addressof(g_process_event));
                }
                process_info->SetExceptionWaitingAttach();
                break;
            case svc::ProcessState_RunningAttached:
                if (process_info->ShouldSignalOnDebugEvent()) {
                    process_info->ClearSuspended();
                    process_info->SetSuspendedStateChanged();
                    os::SignalSystemEvent(std::addressof(g_process_event));
                }
                process_info->ClearUnhandledException();
                break;
            case svc::ProcessState_Terminated:
                /* Unlink from multi wait. */
                os::UnlinkMultiWaitHolder(process_info->GetMultiWaitHolder());

                /* Free process resources. */
                process_info->Cleanup();

                if (hos::GetVersion() < hos::Version_5_0_0 && process_info->ShouldSignalOnExit()) {
                    os::SignalSystemEvent(std::addressof(g_process_event));
                } else {
                    /* Handle the case where we need to keep the process alive some time longer. */
                    if (hos::GetVersion() >= hos::Version_5_0_0 && process_info->ShouldSignalOnExit()) {
                        /* Remove from the living list. */
                        list->Remove(process_info);

                        /* Add the process to the list of dead processes. */
                        {
                            GetExitList()->push_back(*process_info);
                        }

                        /* Signal. */
                        os::SignalSystemEvent(std::addressof(g_process_event));
                    } else {
                        /* Actually delete process. */
                        CleanupProcessInfo(list, process_info);
                    }
                }
                break;
            case svc::ProcessState_DebugBreak:
                if (process_info->ShouldSignalOnDebugEvent()) {
                    process_info->SetSuspended();
                    process_info->SetSuspendedStateChanged();
                    os::SignalSystemEvent(std::addressof(g_process_event));
                }
                break;
        }
    }

    void CreateProcessEvent() {
        /* Create process event. */
        R_ABORT_UNLESS(os::CreateSystemEvent(std::addressof(g_process_event), os::EventClearMode_AutoClear, true));
    }

    Result GetProcessEventHandle(os::NativeHandle *out) {
        *out = os::GetReadableHandleOfSystemEvent(std::addressof(g_process_event));
        R_SUCCEED();
    }

}
