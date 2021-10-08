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
#include "lm_log_server_proxy.hpp"
#include "lm_sd_card_logger.hpp"
#include "lm_log_buffer.hpp"
#include "lm_event_log_transmitter.hpp"

namespace ams::lm::srv {

    bool IsSleeping();

    namespace {

        alignas(os::ThreadStackAlignment) u8 g_flush_thread_stack[8_KB];

        constinit u8 g_fs_heap[32_KB];
        constinit lmem::HeapHandle g_fs_heap_handle;

        constinit os::ThreadType g_flush_thread;

        os::Event g_stop_event(os::EventClearMode_ManualClear);
        os::Event g_sd_logging_event(os::EventClearMode_ManualClear);
        os::Event g_host_connection_event(os::EventClearMode_ManualClear);

        constinit std::unique_ptr<fs::IEventNotifier> g_sd_card_detection_event_notifier;
        os::SystemEvent g_sd_card_detection_event;

        void *AllocateForFs(size_t size) {
            return lmem::AllocateFromExpHeap(g_fs_heap_handle, size);
        }

        void DeallocateForFs(void *ptr, size_t size) {
            AMS_UNUSED(size);
            return lmem::FreeToExpHeap(g_fs_heap_handle, ptr);
        }

        void HostConnectionObserver(bool is_connected) {
            /* Update the host connection event. */
            if (is_connected) {
                g_host_connection_event.Signal();
            } else {
                g_host_connection_event.Clear();

                /* Potentially cancel the log buffer push. */
                if (!g_sd_logging_event.TryWait()) {
                    LogBuffer::GetDefaultInstance().CancelPush();
                }
            }
        }

        void SdLoggingObserver(bool is_available) {
            /* Update the SD card logging event. */
            if (is_available) {
                g_sd_logging_event.Signal();
            } else {
                g_sd_logging_event.Clear();

                /* Potentially cancel the log buffer push. */
                if (!g_host_connection_event.TryWait()) {
                    LogBuffer::GetDefaultInstance().CancelPush();
                }
            }
        }

        bool WaitForFlush() {
            while (true) {
                /* Wait for something to be signaled. */
                os::WaitAny(g_stop_event.GetBase(), g_host_connection_event.GetBase(), g_sd_logging_event.GetBase(), g_sd_card_detection_event.GetBase());

                /* If we're stopping, no flush. */
                if (g_stop_event.TryWait()) {
                    return false;
                }

                /* If host is connected/we're logging to sd, flush. */
                if (g_host_connection_event.TryWait() || g_sd_logging_event.TryWait()) {
                    return true;
                }

                /* If the sd card is newly inserted, flush. */
                if (g_sd_card_detection_event.TryWait()) {
                    g_sd_card_detection_event.Clear();

                    if (fs::IsSdCardInserted()) {
                        return true;
                    }
                }
            }
        }

        void FlushThreadFunction(void *) {
            /* Initialize fs. */
            fs::InitializeWithMultiSessionForSystem();
            fs::SetEnabledAutoAbort(false);

            /* Create fs heap. */
            g_fs_heap_handle = lmem::CreateExpHeap(g_fs_heap, sizeof(g_fs_heap), lmem::CreateOption_None);
            AMS_ABORT_UNLESS(g_fs_heap_handle != nullptr);

            /* Set fs allocator functions. */
            fs::SetAllocator(AllocateForFs, DeallocateForFs);

            /* Create SD card detection event notifier. */
            R_ABORT_UNLESS(fs::OpenSdCardDetectionEventNotifier(std::addressof(g_sd_card_detection_event_notifier)));
            R_ABORT_UNLESS(g_sd_card_detection_event_notifier->BindEvent(g_sd_card_detection_event.GetBase(), os::EventClearMode_ManualClear));

            /* Set connection observers. */
            SdCardLogger::GetInstance().SetLoggingObserver(SdLoggingObserver);
            LogServerProxy::GetInstance().SetConnectionObserver(HostConnectionObserver);

            /* Do flush loop. */
            do {
                if (LogBuffer::GetDefaultInstance().Flush()) {
                    EventLogTransmitter::GetDefaultInstance().PushLogPacketDropCountIfExists();
                }
            } while (WaitForFlush());

            /* Clear connection observer. */
            LogServerProxy::GetInstance().SetConnectionObserver(nullptr);

            /* Finalize the SD card logger. */
            SdCardLogger::GetInstance().Finalize();
            SdCardLogger::GetInstance().SetLoggingObserver(nullptr);

            /* Destroy the fs heap. */
            lmem::DestroyExpHeap(g_fs_heap_handle);
        }

    }

    bool IsFlushAvailable() {
        /* If we're sleeping, we can't flush. */
        if (IsSleeping()) {
            return false;
        }

        /* Try to wait for an event. */
        if (os::TryWaitAny(g_stop_event.GetBase(), g_host_connection_event.GetBase(), g_sd_logging_event.GetBase()) < 0) {
            return false;
        }

        /* Return whether we're not stopping. */
        return !os::TryWaitEvent(g_stop_event.GetBase());
    }

    void InitializeFlushThread() {
        /* Create the flush thread. */
        R_ABORT_UNLESS(os::CreateThread(std::addressof(g_flush_thread), FlushThreadFunction, nullptr, g_flush_thread_stack, sizeof(g_flush_thread_stack), AMS_GET_SYSTEM_THREAD_PRIORITY(lm, Flush)));
        os::SetThreadNamePointer(std::addressof(g_flush_thread), AMS_GET_SYSTEM_THREAD_NAME(lm, Flush));

        /* Clear the stop event. */
        g_stop_event.Clear();

        /* Start the flush thread. */
        os::StartThread(std::addressof(g_flush_thread));
    }

    void FinalizeFlushThread() {
        /* Signal the flush thread to stop. */
        g_stop_event.Signal();

        /* Wait for the flush thread to stop. */
        os::WaitThread(std::addressof(g_flush_thread));
        os::DestroyThread(std::addressof(g_flush_thread));
    }

}
