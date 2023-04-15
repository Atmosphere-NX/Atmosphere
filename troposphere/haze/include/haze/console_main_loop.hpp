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
            static constexpr size_t FrameDelayNs = 33333333;

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
                    Waiter cancel_waiter = waiterForUEvent(std::addressof(m_cancel_event));
                    Result rc = waitObjects(std::addressof(idx), std::addressof(cancel_waiter), 1, FrameDelayNs);

                    if (R_SUCCEEDED(rc)) break;
                    if (svc::ResultTimedOut::Includes(rc)) ueventSignal(std::addressof(m_event));
                }
            }

        public:
            explicit ConsoleMainLoop() : m_reactor(), m_pad(), m_thread(), m_event(), m_cancel_event(), m_last_heap_used(), m_last_heap_total(), m_is_applet_mode() { /* ... */ }

            Result Initialize(EventReactor *reactor, PtpObjectHeap *object_heap) {
                m_reactor = reactor;
                m_object_heap = object_heap;

                AppletType applet_type = appletGetAppletType();
                m_is_applet_mode = applet_type != AppletType_Application && applet_type != AppletType_SystemApplication;

                ueventCreate(std::addressof(m_event), true);
                ueventCreate(std::addressof(m_cancel_event), true);

                /* Set up pad inputs to allow exiting the program. */
                padConfigureInput(1, HidNpadStyleSet_NpadStandard);
                padInitializeAny(std::addressof(m_pad));

                /* Create the delay thread with higher priority than the main thread. */
                R_TRY(threadCreate(std::addressof(m_thread), ConsoleMainLoop::Run, this, nullptr, 0x1000, 0x2b, -2));

                /* Ensure we close the thread on failure. */
                ON_RESULT_FAILURE { threadClose(std::addressof(m_thread)); };

                /* Connect ourselves to the event loop. */
                R_UNLESS(m_reactor->AddConsumer(this, waiterForUEvent(std::addressof(m_event))), haze::ResultRegistrationFailed());

                /* Start the delay thread. */
                R_RETURN(threadStart(std::addressof(m_thread)));
            }

            void Finalize() {
                ueventSignal(std::addressof(m_cancel_event));

                HAZE_R_ABORT_UNLESS(threadWaitForExit(std::addressof(m_thread)));

                HAZE_R_ABORT_UNLESS(threadClose(std::addressof(m_thread)));

                m_reactor->RemoveConsumer(this);
            }

        private:
            void RedrawConsole() {
                u32 heap_used = m_object_heap->GetSizeUsed();
                u32 heap_total = m_object_heap->GetSizeTotal();
                u32 heap_pct = heap_total > 0 ? static_cast<u32>((heap_used * 100ul) / heap_total) : 0;

                if (heap_used == m_last_heap_used && heap_total == m_last_heap_total) {
                    /* If usage didn't change, skip redrawing the console. */
                    /* This provides a substantial performance improvement in file transfer speed. */
                    return;
                }

                m_last_heap_used = heap_used;
                m_last_heap_total = heap_total;

                const char *used_unit = "B";
                if (heap_used > 1024) { heap_used >>= 10; used_unit = "KiB"; }
                if (heap_used > 1024) { heap_used >>= 10; used_unit = "MiB"; }

                const char *total_unit = "B";
                if (heap_total > 1024) { heap_total >>= 10; total_unit = "KiB"; }
                if (heap_total > 1024) { heap_total >>= 10; total_unit = "MiB"; }

                consoleClear();
                printf("USB File Transfer\n\n");
                printf("Connect console to computer. Press [+] to exit.\n");
                printf("Heap used: %u %s / %u %s (%u%%)\n", heap_used, used_unit, heap_total, total_unit, heap_pct);

                if (m_is_applet_mode) {
                    printf("\n" CONSOLE_ESC(38;5;196m) "Applet Mode" CONSOLE_ESC(0m) "\n");
                }

                consoleUpdate(NULL);
            }

        protected:
            void ProcessEvent() override {
                this->RedrawConsole();

                /* Check buttons. */
                padUpdate(std::addressof(m_pad));

                if (padGetButtonsDown(std::addressof(m_pad)) & HidNpadButton_Plus) {
                    m_reactor->RequestStop();
                }

                /* Pump applet event loop. */
                if (!appletMainLoop()) {
                    m_reactor->RequestStop();
                }
            }
    };

}
