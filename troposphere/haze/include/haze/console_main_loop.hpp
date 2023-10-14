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
#pragma once

#include <haze/event_reactor.hpp>
#include <haze/ptp_object_heap.hpp>

namespace haze {

    class ConsoleMainLoop : public EventConsumer {
        private:
            static constexpr size_t FrameDelayNs = 33'333'333;
        private:
            EventReactor *m_reactor;
            PtpObjectHeap *m_object_heap;

            PadState m_pad;

            Thread m_thread;
            UEvent m_event;
            UEvent m_cancel_event;

            u32 m_last_heap_used;
            u32 m_last_heap_total;
            bool m_is_applet_mode;
        private:
            static void Run(void *arg) {
                static_cast<ConsoleMainLoop *>(arg)->Run();
            }

            void Run() {
                int idx;

                while (true) {
                    /* Wait for up to 1 frame delay time to be cancelled. */
                    Waiter cancel_waiter = waiterForUEvent(std::addressof(m_cancel_event));
                    Result rc = waitObjects(std::addressof(idx), std::addressof(cancel_waiter), 1, FrameDelayNs);

                    /* Finish if we were cancelled. */
                    if (R_SUCCEEDED(rc)) {
                        break;
                    }

                    /* Otherwise, signal the console update event. */
                    if (svc::ResultTimedOut::Includes(rc)) {
                        ueventSignal(std::addressof(m_event));
                    }
                }
            }
        public:
            explicit ConsoleMainLoop() : m_reactor(), m_pad(), m_thread(), m_event(), m_cancel_event(), m_last_heap_used(), m_last_heap_total(), m_is_applet_mode() { /* ... */ }

            Result Initialize(EventReactor *reactor, PtpObjectHeap *object_heap) {
                /* Register event reactor and heap. */
                m_reactor     = reactor;
                m_object_heap = object_heap;

                /* Set cached use amounts to invalid values. */
                m_last_heap_used  = 0xffffffffu;
                m_last_heap_total = 0xffffffffu;

                /* Get whether we are launched in applet mode. */
                AppletType applet_type = appletGetAppletType();
                m_is_applet_mode = applet_type != AppletType_Application && applet_type != AppletType_SystemApplication;

                /* Initialize events. */
                ueventCreate(std::addressof(m_event), true);
                ueventCreate(std::addressof(m_cancel_event), true);

                /* Set up pad inputs to allow exiting the program. */
                padConfigureInput(1, HidNpadStyleSet_NpadStandard);
                padInitializeAny(std::addressof(m_pad));

                /* Create the delay thread with higher priority than the main thread (which runs at priority 0x2c). */
                R_TRY(threadCreate(std::addressof(m_thread), ConsoleMainLoop::Run, this, nullptr, 4_KB, 0x2b, svc::IdealCoreUseProcessValue));

                /* Ensure we close the thread on failure. */
                ON_RESULT_FAILURE { threadClose(std::addressof(m_thread)); };

                /* Connect ourselves to the event loop. */
                R_UNLESS(m_reactor->AddConsumer(this, waiterForUEvent(std::addressof(m_event))), haze::ResultRegistrationFailed());

                /* Start the delay thread. */
                R_RETURN(threadStart(std::addressof(m_thread)));
            }

            void Finalize() {
                /* Signal the delay thread to shut down. */
                ueventSignal(std::addressof(m_cancel_event));

                /* Wait for the delay thread to exit and close it. */
                HAZE_R_ABORT_UNLESS(threadWaitForExit(std::addressof(m_thread)));

                HAZE_R_ABORT_UNLESS(threadClose(std::addressof(m_thread)));

                /* Disconnect from the event loop.*/
                m_reactor->RemoveConsumer(this);
            }
        private:
            void RedrawConsole() {
                /* Get use amounts from the heap. */
                u32 heap_used  = m_object_heap->GetUsedSize();
                u32 heap_total = m_object_heap->GetTotalSize();
                u32 heap_pct   = heap_total > 0 ? static_cast<u32>((heap_used * 100ul) / heap_total) : 0;

                if (heap_used == m_last_heap_used && heap_total == m_last_heap_total) {
                    /* If usage didn't change, skip redrawing the console. */
                    /* This provides a substantial performance improvement in file transfer speed. */
                    return;
                }

                /* Update cached use amounts. */
                m_last_heap_used  = heap_used;
                m_last_heap_total = heap_total;

                /* Determine units to use for printing to the console. */
                const char *used_unit = "B";
                if (heap_used >= 1_KB)          { heap_used >>= 10; used_unit = "KiB"; }
                if (heap_used >= (1_MB / 1_KB)) { heap_used >>= 10; used_unit = "MiB"; }

                const char *total_unit = "B";
                if (heap_total >= 1_KB)          { heap_total >>= 10; total_unit = "KiB"; }
                if (heap_total >= (1_MB / 1_KB)) { heap_total >>= 10; total_unit = "MiB"; }

                /* Draw the console UI. */
                consoleClear();
                printf("USB File Transfer\n\n");
                printf("Connect console to computer. Press [+] to exit.\n");
                printf("Heap used: %u %s / %u %s (%u%%)\n", heap_used, used_unit, heap_total, total_unit, heap_pct);

                if (m_is_applet_mode) {
                    /* Print "Applet Mode" in red text. */
                    printf("\n" CONSOLE_ESC(31;1m) "Applet Mode" CONSOLE_ESC(0m) "\n");
                }

                consoleUpdate(nullptr);
            }
        protected:
            void ProcessEvent() override {
                /* Update the console. */
                this->RedrawConsole();

                /* Check buttons. */
                padUpdate(std::addressof(m_pad));

                /* If the plus button is held, request immediate exit. */
                if (padGetButtonsDown(std::addressof(m_pad)) & HidNpadButton_Plus) {
                    m_reactor->SetResult(haze::ResultStopRequested());
                }

                /* Pump applet events, and check if exit was requested. */
                if (!appletMainLoop()) {
                    m_reactor->SetResult(haze::ResultStopRequested());
                }

                /* Check if focus was lost. */
                if (appletGetFocusState() == AppletFocusState_Background) {
                    m_reactor->SetResult(haze::ResultFocusLost());
                }
            }
        private:
            static bool SuspendAndWaitForFocus() {
                /* Enable suspension with resume notification. */
                appletSetFocusHandlingMode(AppletFocusHandlingMode_SuspendHomeSleepNotify);

                /* Pump applet events. */
                while (appletMainLoop()) {
                    /* Check if focus was regained. */
                    if (appletGetFocusState() != AppletFocusState_Background) {
                        return true;
                    }
                }

                /* Exit was requested. */
                return false;
            }
        public:
            static void RunApplication() {
                /* Declare the object heap, to hold the database for an active session. */
                PtpObjectHeap ptp_object_heap;

                /* Declare the event reactor, and components which use it. */
                EventReactor event_reactor;
                PtpResponder ptp_responder;
                ConsoleMainLoop console_main_loop;

                /* Initialize the console.*/
                consoleInit(nullptr);

                while (true) {
                    /* Disable suspension. */
                    appletSetFocusHandlingMode(AppletFocusHandlingMode_NoSuspend);

                    /* Declare result from serving to use. */
                    Result rc;
                    {
                        /* Ensure we don't go to sleep while transferring files. */
                        appletSetAutoSleepDisabled(true);

                        /* Clear the event reactor. */
                        event_reactor.SetResult(ResultSuccess());

                        /* Configure the PTP responder and console main loop. */
                        ptp_responder.Initialize(std::addressof(event_reactor), std::addressof(ptp_object_heap));
                        console_main_loop.Initialize(std::addressof(event_reactor), std::addressof(ptp_object_heap));

                        /* Ensure we maintain a clean state on exit. */
                        ON_SCOPE_EXIT {
                            /* Finalize the console main loop and PTP responder. */
                            console_main_loop.Finalize();
                            ptp_responder.Finalize();

                            /* Restore auto sleep setting. */
                            appletSetAutoSleepDisabled(false);
                        };

                        /* Begin processing requests. */
                        rc = ptp_responder.LoopProcess();
                    }

                    /* If focus was lost, try to pump the applet main loop until we receive focus again. */
                    if (haze::ResultFocusLost::Includes(rc) && SuspendAndWaitForFocus()) {
                        continue;
                    }

                    /* Otherwise, enable suspension and finish. */
                    appletSetFocusHandlingMode(AppletFocusHandlingMode_SuspendHomeSleep);
                    break;
                }

                /* Finalize the console. */
                consoleExit(nullptr);
            }
    };

}
