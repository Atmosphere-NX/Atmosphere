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

namespace ams {

    namespace {

        struct InterThreadSync {
            util::Atomic<int> reader_state;
            util::Atomic<int> writer_state;
            os::EventType writer_ready_event;
            os::EventType reader_ready_event;
            union {
                struct {
                    os::SystemEventType system_event_as_manual_clear_event;
                    os::SystemEventType system_event_as_manual_clear_interprocess_event;
                    os::SystemEventType system_event_as_auto_clear_event;
                    os::SystemEventType system_event_as_auto_clear_interprocess_event;
                };
                os::SystemEventType system_events[4];
            };
        };

        bool IsManualClearEventIndex(size_t i) {
            return i == 0 || i == 1;
        }

        alignas(os::MemoryPageSize) constinit u8 g_writer_thread_stack[16_KB];
        alignas(os::MemoryPageSize) constinit u8 g_reader_thread_stack[16_KB];

        void TestWriterThread(void *arg) {
            /* Get the synchronization arguments. */
            auto &sync = *static_cast<InterThreadSync *>(arg);
            AMS_UNUSED(sync);

            /* Wait for reader to be ready. */
            os::WaitEvent(std::addressof(sync.reader_ready_event));
            AMS_ABORT_UNLESS(sync.reader_state == 1);

            /* Verify that all events can be signaled. */
            for (size_t i = 0; i < util::size(sync.system_events); ++i) {
                /* Set the event for this go. */
                os::SignalSystemEvent(sync.system_events + i);

                sync.writer_state = 1;
                os::SignalEvent(std::addressof(sync.writer_ready_event));

                /* Wait for the reader to finish. */
                os::WaitEvent(std::addressof(sync.reader_ready_event));
                AMS_ABORT_UNLESS(sync.reader_state == 1);
            }

            /* Verify that all events can be signaled (for TimedWait 0). */
            for (size_t i = 0; i < util::size(sync.system_events); ++i) {
                /* Set the event for this go. */
                os::SignalSystemEvent(sync.system_events + i);

                sync.writer_state = 2;
                os::SignalEvent(std::addressof(sync.writer_ready_event));

                /* Wait for the reader to finish. */
                os::WaitEvent(std::addressof(sync.reader_ready_event));
                AMS_ABORT_UNLESS(sync.reader_state == 2);
            }

            /* Verify that all events can be signaled (for TimedWait 2). */
            for (size_t i = 0; i < util::size(sync.system_events); ++i) {
                /* Set the event for this go. */
                os::SignalSystemEvent(sync.system_events + i);

                sync.writer_state = 3;
                os::SignalEvent(std::addressof(sync.writer_ready_event));

                /* Wait for the reader to finish. */
                os::WaitEvent(std::addressof(sync.reader_ready_event));
                AMS_ABORT_UNLESS(sync.reader_state == 3);
            }

            /* Verify that all events can be signaled (for True Wait). */
            for (size_t i = 0; i < util::size(sync.system_events); ++i) {
                /* Set the event for this go. */
                os::SignalSystemEvent(sync.system_events + i);

                sync.writer_state = 4;
                os::SignalEvent(std::addressof(sync.writer_ready_event));

                /* Wait for the reader to finish. */
                os::WaitEvent(std::addressof(sync.reader_ready_event));
                AMS_ABORT_UNLESS(sync.reader_state == 4);
            }

            /* Verify that all events can be signaled (TryWaitAny). */
            for (size_t i = 0; i < util::size(sync.system_events); ++i) {
                /* Set the event for this go. */
                os::SignalSystemEvent(sync.system_events + i);

                sync.writer_state = 5;
                os::SignalEvent(std::addressof(sync.writer_ready_event));

                /* Wait for the reader to finish. */
                os::WaitEvent(std::addressof(sync.reader_ready_event));
                AMS_ABORT_UNLESS(sync.reader_state == 5);
            }

            /* Verify that all events can be signaled (TimedWaitAny 0). */
            for (size_t i = 0; i < util::size(sync.system_events); ++i) {
                /* Set the event for this go. */
                os::SignalSystemEvent(sync.system_events + i);

                sync.writer_state = 6;
                os::SignalEvent(std::addressof(sync.writer_ready_event));

                /* Wait for the reader to finish. */
                os::WaitEvent(std::addressof(sync.reader_ready_event));
                AMS_ABORT_UNLESS(sync.reader_state == 6);
            }

            /* Verify that all events can be signaled (TimedWaitAny 2). */
            for (size_t i = 0; i < util::size(sync.system_events); ++i) {
                /* Set the event for this go. */
                os::SignalSystemEvent(sync.system_events + i);

                sync.writer_state = 7;
                os::SignalEvent(std::addressof(sync.writer_ready_event));

                /* Wait for the reader to finish. */
                os::WaitEvent(std::addressof(sync.reader_ready_event));
                AMS_ABORT_UNLESS(sync.reader_state == 7);
            }

            /* Verify that all events can be signaled (TrueWaitAny). */
            for (size_t i = 0; i < util::size(sync.system_events); ++i) {
                /* Set the event for this go. */
                os::SignalSystemEvent(sync.system_events + i);

                sync.writer_state = 8;
                os::SignalEvent(std::addressof(sync.writer_ready_event));

                /* Wait for the reader to finish. */
                os::WaitEvent(std::addressof(sync.reader_ready_event));
                AMS_ABORT_UNLESS(sync.reader_state == 8);
            }

            /* Verify that reader can receive without explicit sync. */
            for (size_t i = 0; i < util::size(sync.system_events); ++i) {
                /* Set the event for this go. */
                os::SignalSystemEvent(sync.system_events + i);

                sync.writer_state = 9;

            }

            /* Wait for the reader to finish. */
            os::WaitEvent(std::addressof(sync.reader_ready_event));
            AMS_ABORT_UNLESS(sync.reader_state == 9);
        }

        void TestReaderThread(void *arg) {
            /* Get the synchronization arguments. */
            auto &sync = *static_cast<InterThreadSync *>(arg);
            AMS_UNUSED(sync);

            /* Set up multi-wait objects. */
            os::MultiWaitType mw;
            os::MultiWaitHolderType holders[util::size(sync.system_events)];
            os::InitializeMultiWait(std::addressof(mw));
            for (size_t i = 0; i < util::size(sync.system_events); ++i) {
                os::InitializeMultiWaitHolder(holders + i, sync.system_events + i);
                os::LinkMultiWaitHolder(std::addressof(mw), holders + i);
            }
            ON_SCOPE_EXIT {
                for (size_t i = 0; i < util::size(sync.system_events); ++i) {
                    os::UnlinkMultiWaitHolder(holders + i);
                    os::FinalizeMultiWaitHolder(holders + i);
                }
                os::FinalizeMultiWait(std::addressof(mw));
            };

            /* Sanity check: all events are non-signaled. */
            for (size_t i = 0; i < util::size(sync.system_events); ++i) {
                AMS_ABORT_UNLESS(os::TryWaitSystemEvent(sync.system_events + i) == false);
                AMS_ABORT_UNLESS(os::TimedWaitSystemEvent(sync.system_events + i, TimeSpan::FromNanoSeconds(0)) == false);
                AMS_ABORT_UNLESS(os::TimedWaitSystemEvent(sync.system_events + i, TimeSpan::FromMilliSeconds(2)) == false);
            }

            /* Sanity check that wait any does the right thing when nothing is signaled. */
            AMS_ABORT_UNLESS(os::TryWaitAny(std::addressof(mw)) == nullptr);
            AMS_ABORT_UNLESS(os::TimedWaitAny(std::addressof(mw), TimeSpan::FromNanoSeconds(0)) == nullptr);
            AMS_ABORT_UNLESS(os::TimedWaitAny(std::addressof(mw), TimeSpan::FromNanoSeconds(2)) == nullptr);

            /* Let writer know that we're ready. */
            sync.reader_state = 1;
            os::SignalEvent(std::addressof(sync.reader_ready_event));

            /* Verify that we can receive signal on each event. */
            for (size_t i = 0; i < util::size(sync.system_events); ++i) {
                /* Wait for writer to do the relevant work */
                os::WaitEvent(std::addressof(sync.writer_ready_event));
                AMS_ABORT_UNLESS(sync.writer_state == 1);

                /* Test all events. */
                for (size_t n = 0; n < util::size(sync.system_events); ++n) {
                    if (i == n) {
                        AMS_ABORT_UNLESS(os::TryWaitSystemEvent(sync.system_events + n) == true);
                        if (IsManualClearEventIndex(n)) {
                            AMS_ABORT_UNLESS(os::TryWaitSystemEvent(sync.system_events + n) == true);
                            os::ClearSystemEvent(sync.system_events + n);
                            AMS_ABORT_UNLESS(os::TryWaitSystemEvent(sync.system_events + n) == false);
                        } else {
                            AMS_ABORT_UNLESS(os::TryWaitSystemEvent(sync.system_events + n) == false);
                        }
                    } else {
                        AMS_ABORT_UNLESS(os::TryWaitSystemEvent(sync.system_events + n) == false);
                    }
                }

                /* Let writer know we're done. */
                sync.reader_state = 1;
                os::SignalEvent(std::addressof(sync.reader_ready_event));
            }

            /* Verify that we can receive signal on each event (Timed Wait 0). */
            for (size_t i = 0; i < util::size(sync.system_events); ++i) {
                /* Wait for writer to do the relevant work */
                os::WaitEvent(std::addressof(sync.writer_ready_event));
                AMS_ABORT_UNLESS(sync.writer_state == 2);

                /* Test all events. */
                for (size_t n = 0; n < util::size(sync.system_events); ++n) {
                    if (i == n) {
                        AMS_ABORT_UNLESS(os::TimedWaitSystemEvent(sync.system_events + n, TimeSpan::FromMilliSeconds(0)) == true);
                        if (IsManualClearEventIndex(n)) {
                            AMS_ABORT_UNLESS(os::TimedWaitSystemEvent(sync.system_events + n, TimeSpan::FromMilliSeconds(0)) == true);
                            os::ClearSystemEvent(sync.system_events + n);
                            AMS_ABORT_UNLESS(os::TimedWaitSystemEvent(sync.system_events + n, TimeSpan::FromMilliSeconds(0)) == false);
                        } else {
                            AMS_ABORT_UNLESS(os::TimedWaitSystemEvent(sync.system_events + n, TimeSpan::FromMilliSeconds(0)) == false);
                        }
                    } else {
                        AMS_ABORT_UNLESS(os::TimedWaitSystemEvent(sync.system_events + n, TimeSpan::FromMilliSeconds(0)) == false);
                    }
                }

                /* Let writer know we're done. */
                sync.reader_state = 2;
                os::SignalEvent(std::addressof(sync.reader_ready_event));
            }

            /* Verify that we can receive signal on each event (Timed Wait 2). */
            for (size_t i = 0; i < util::size(sync.system_events); ++i) {
                /* Wait for writer to do the relevant work */
                os::WaitEvent(std::addressof(sync.writer_ready_event));
                AMS_ABORT_UNLESS(sync.writer_state == 3);

                /* Test all events. */
                for (size_t n = 0; n < util::size(sync.system_events); ++n) {
                    if (i == n) {
                        AMS_ABORT_UNLESS(os::TimedWaitSystemEvent(sync.system_events + n, TimeSpan::FromMilliSeconds(2)) == true);
                        if (IsManualClearEventIndex(n)) {
                            AMS_ABORT_UNLESS(os::TimedWaitSystemEvent(sync.system_events + n, TimeSpan::FromMilliSeconds(2)) == true);
                            os::ClearSystemEvent(sync.system_events + n);
                            AMS_ABORT_UNLESS(os::TimedWaitSystemEvent(sync.system_events + n, TimeSpan::FromMilliSeconds(2)) == false);
                        } else {
                            AMS_ABORT_UNLESS(os::TimedWaitSystemEvent(sync.system_events + n, TimeSpan::FromMilliSeconds(2)) == false);
                        }
                    } else {
                        AMS_ABORT_UNLESS(os::TimedWaitSystemEvent(sync.system_events + n, TimeSpan::FromMilliSeconds(2)) == false);
                    }
                }

                /* Let writer know we're done. */
                sync.reader_state = 3;
                os::SignalEvent(std::addressof(sync.reader_ready_event));
            }

            /* Verify that we can receive signal on each event. */
            for (size_t i = 0; i < util::size(sync.system_events); ++i) {
                /* Wait for writer to do the relevant work */
                os::WaitEvent(std::addressof(sync.writer_ready_event));
                AMS_ABORT_UNLESS(sync.writer_state == 4);

                /* Test all events. */
                for (size_t n = 0; n < util::size(sync.system_events); ++n) {
                    if (i == n) {
                        os::WaitSystemEvent(sync.system_events + n);
                        if (IsManualClearEventIndex(n)) {
                            AMS_ABORT_UNLESS(os::TryWaitSystemEvent(sync.system_events + n) == true);
                            os::WaitSystemEvent(sync.system_events + n);
                            os::ClearSystemEvent(sync.system_events + n);
                            AMS_ABORT_UNLESS(os::TryWaitSystemEvent(sync.system_events + n) == false);
                        } else {
                            AMS_ABORT_UNLESS(os::TryWaitSystemEvent(sync.system_events + n) == false);
                        }
                    } else {
                        AMS_ABORT_UNLESS(os::TryWaitSystemEvent(sync.system_events + n) == false);
                    }
                }

                /* Let writer know we're done. */
                sync.reader_state = 4;
                os::SignalEvent(std::addressof(sync.reader_ready_event));
            }

            /* Verify that we can receive signal on each event (TryWaitAny) */
            for (size_t i = 0; i < util::size(sync.system_events); ++i) {
                /* Wait for writer to do the relevant work */
                os::WaitEvent(std::addressof(sync.writer_ready_event));
                AMS_ABORT_UNLESS(sync.writer_state == 5);

                /* Get the signaled holder. */
                auto *signaled = os::TryWaitAny(std::addressof(mw));
                AMS_ABORT_UNLESS(signaled == holders + i);

                /* Test all events. */
                for (size_t n = 0; n < util::size(sync.system_events); ++n) {
                    AMS_ABORT_UNLESS(os::TryWaitSystemEvent(sync.system_events + n) == (i == n));
                    os::ClearSystemEvent(sync.system_events + n);
                    AMS_ABORT_UNLESS(os::TryWaitSystemEvent(sync.system_events + n) == false);
                }

                /* Let writer know we're done. */
                sync.reader_state = 5;
                os::SignalEvent(std::addressof(sync.reader_ready_event));
            }

            /* Verify that we can receive signal on each event (TimedWaitAny 0) */
            for (size_t i = 0; i < util::size(sync.system_events); ++i) {
                /* Wait for writer to do the relevant work */
                os::WaitEvent(std::addressof(sync.writer_ready_event));
                AMS_ABORT_UNLESS(sync.writer_state == 6);

                /* Get the signaled holder. */
                auto *signaled = os::TimedWaitAny(std::addressof(mw), TimeSpan::FromMilliSeconds(0));
                AMS_ABORT_UNLESS(signaled == holders + i);

                /* Test all events. */
                for (size_t n = 0; n < util::size(sync.system_events); ++n) {
                    AMS_ABORT_UNLESS(os::TryWaitSystemEvent(sync.system_events + n) == (i == n));
                    os::ClearSystemEvent(sync.system_events + n);
                    AMS_ABORT_UNLESS(os::TryWaitSystemEvent(sync.system_events + n) == false);
                }

                /* Let writer know we're done. */
                sync.reader_state = 6;
                os::SignalEvent(std::addressof(sync.reader_ready_event));
            }

            /* Verify that we can receive signal on each event (TimedWaitAny 2) */
            for (size_t i = 0; i < util::size(sync.system_events); ++i) {
                /* Wait for writer to do the relevant work */
                os::WaitEvent(std::addressof(sync.writer_ready_event));
                AMS_ABORT_UNLESS(sync.writer_state == 7);

                /* Get the signaled holder. */
                auto *signaled = os::TimedWaitAny(std::addressof(mw), TimeSpan::FromMilliSeconds(2));
                AMS_ABORT_UNLESS(signaled == holders + i);

                /* Test all events. */
                for (size_t n = 0; n < util::size(sync.system_events); ++n) {
                    AMS_ABORT_UNLESS(os::TryWaitSystemEvent(sync.system_events + n) == (i == n));
                    os::ClearSystemEvent(sync.system_events + n);
                    AMS_ABORT_UNLESS(os::TryWaitSystemEvent(sync.system_events + n) == false);
                }

                /* Let writer know we're done. */
                sync.reader_state = 7;
                os::SignalEvent(std::addressof(sync.reader_ready_event));
            }

            /* Verify that we can receive signal on each event (True WaitAny) */
            for (size_t i = 0; i < util::size(sync.system_events); ++i) {
                /* Wait for writer to do the relevant work */
                os::WaitEvent(std::addressof(sync.writer_ready_event));
                AMS_ABORT_UNLESS(sync.writer_state == 8);

                /* Get the signaled holder. */
                auto *signaled = os::WaitAny(std::addressof(mw));
                AMS_ABORT_UNLESS(signaled == holders + i);

                /* Test all events. */
                for (size_t n = 0; n < util::size(sync.system_events); ++n) {
                    AMS_ABORT_UNLESS(os::TryWaitSystemEvent(sync.system_events + n) == (i == n));
                    os::ClearSystemEvent(sync.system_events + n);
                    AMS_ABORT_UNLESS(os::TryWaitSystemEvent(sync.system_events + n) == false);
                }

                /* Let writer know we're done. */
                sync.reader_state = 8;
                os::SignalEvent(std::addressof(sync.reader_ready_event));
            }

            /* Verify that we can receive wait-any signals without sync. */
            for (size_t i = 0; i < util::size(sync.system_events); ++i) {
                auto *signaled = os::WaitAny(std::addressof(mw));
                AMS_ABORT_UNLESS(signaled != nullptr);
                const size_t n = signaled - holders;
                AMS_ABORT_UNLESS(n < util::size(sync.system_events));

                AMS_ABORT_UNLESS(os::TryWaitSystemEvent(sync.system_events + n) == true);
                os::ClearSystemEvent(sync.system_events + n);
                AMS_ABORT_UNLESS(os::TryWaitSystemEvent(sync.system_events + n) == false);
            }

            AMS_ABORT_UNLESS(os::TryWaitAny(std::addressof(mw)) == nullptr);

            /* Let writer know we're done. */
            sync.reader_state = 9;
            os::SignalEvent(std::addressof(sync.reader_ready_event));
        }

    }


    void Main() {
        printf("Doing OS Event tests!\n");
        {
            /* Create the synchronization state. */
            InterThreadSync sync_state;
            sync_state.reader_state = 0;
            sync_state.writer_state = 0;
            os::InitializeEvent(std::addressof(sync_state.writer_ready_event), false, os::EventClearMode_AutoClear);
            os::InitializeEvent(std::addressof(sync_state.reader_ready_event), false, os::EventClearMode_AutoClear);

            R_ABORT_UNLESS(os::CreateSystemEvent(std::addressof(sync_state.system_event_as_manual_clear_event), os::EventClearMode_ManualClear, false));
            R_ABORT_UNLESS(os::CreateSystemEvent(std::addressof(sync_state.system_event_as_manual_clear_interprocess_event), os::EventClearMode_ManualClear, true));
            R_ABORT_UNLESS(os::CreateSystemEvent(std::addressof(sync_state.system_event_as_auto_clear_event), os::EventClearMode_AutoClear, false));
            R_ABORT_UNLESS(os::CreateSystemEvent(std::addressof(sync_state.system_event_as_auto_clear_interprocess_event), os::EventClearMode_AutoClear, true));

            /* Ensure we clean up the sync-state when done. */
            ON_SCOPE_EXIT {
                os::FinalizeEvent(std::addressof(sync_state.writer_ready_event));
                os::FinalizeEvent(std::addressof(sync_state.reader_ready_event));

                os::DestroySystemEvent(std::addressof(sync_state.system_event_as_manual_clear_event));
                os::DestroySystemEvent(std::addressof(sync_state.system_event_as_manual_clear_interprocess_event));
                os::DestroySystemEvent(std::addressof(sync_state.system_event_as_auto_clear_event));
                os::DestroySystemEvent(std::addressof(sync_state.system_event_as_auto_clear_interprocess_event));
            };

            /* Create the threads. */
            os::ThreadType reader_thread, writer_thread;
            R_ABORT_UNLESS(os::CreateThread(std::addressof(reader_thread), TestReaderThread, std::addressof(sync_state), g_reader_thread_stack, sizeof(g_reader_thread_stack), os::DefaultThreadPriority));
            R_ABORT_UNLESS(os::CreateThread(std::addressof(writer_thread), TestWriterThread, std::addressof(sync_state), g_writer_thread_stack, sizeof(g_writer_thread_stack), os::DefaultThreadPriority));
            os::SetThreadNamePointer(std::addressof(reader_thread), "ReaderThread");
            os::SetThreadNamePointer(std::addressof(writer_thread), "WriterThread");

            /* Start the threads. */
            os::StartThread(std::addressof(reader_thread));
            os::StartThread(std::addressof(writer_thread));

            /* Wait for the threads to complete. */
            os::WaitThread(std::addressof(reader_thread));
            os::WaitThread(std::addressof(writer_thread));

            /* Destroy the threads. */
            os::WaitThread(std::addressof(reader_thread));
            os::WaitThread(std::addressof(writer_thread));
        }
        printf("All tests completed!\n");
    }

}