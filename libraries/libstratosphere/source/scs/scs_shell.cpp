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

namespace ams::scs {

    namespace {

        struct SocketInfo {
            u64 id;
            s32 socket;
        };

        struct ProgramInfo {
            os::ProcessId process_id;
            u64 id;
            s32 socket;
            s32 info_id;
            bool started;
            bool jit_debug;
            bool launched_by_cs;
        };

        constexpr inline auto MaxSocketInfo  = 2;
        constexpr inline auto MaxProgramInfo = 64;

        class SocketInfoManager {
            private:
                SocketInfo m_infos[MaxSocketInfo];
                int m_count;
            public:
                constexpr SocketInfoManager() = default;

                void Initialize() {
                    /* Clear our count. */
                    m_count = 0;
                }

                Result Register(s32 socket, u64 id) {
                    /* Check that the socket isn't already registered. */
                    for (auto i = 0; i < m_count; ++i) {
                        R_UNLESS(m_infos[i].socket != socket, scs::ResultNoSocket());
                    }

                    /* Check that we can allocate a new socket info. */
                    if (m_count >= MaxSocketInfo) {
                        /* NOTE: Nintendo aborts with this result here. */
                        R_ABORT_UNLESS(scs::ResultOutOfResource());
                    }

                    /* Create the new socket info. */
                    m_infos[m_count++] = {
                        .id     = id,
                        .socket = socket,
                    };

                    return ResultSuccess();
                }

                void Unregister(s32 socket) {
                    /* Unregister the socket, if it's registered. */
                    for (auto i = 0; i < m_count; ++i) {
                        if (m_infos[i].socket == socket) {
                            /* Ensure that the valid socket infos remain in bounds. */
                            std::memcpy(m_infos + i, m_infos + i + 1, (m_count - (i + 1)) * sizeof(*m_infos));

                            /* Note that we now have one fewer socket info. */
                            --m_count;

                            break;
                        }
                    }
                }

                void InvokeHandler(ProcessEventHandler handler, os::ProcessId process_id) {
                    /* Invoke the handler on all our sockets. */
                    for (auto i = 0; i < m_count; ++i) {
                        handler(m_infos[i].id, m_infos[i].socket, process_id);
                    }
                }
        };

        class ProgramInfoManager {
            private:
                s32 m_next_info_id;
                ProgramInfo m_infos[MaxProgramInfo];
                int m_count;
            public:
                constexpr ProgramInfoManager() = default;

                void Initialize() {
                    /* Reset our next id. */
                    m_next_info_id = 1;

                    /* Clear our count. */
                    m_count = 0;
                }

                const ProgramInfo *Find(os::ProcessId process_id) const {
                    /* Find a matching program. */
                    for (auto i = 0; i < m_count; ++i) {
                        if (m_infos[i].process_id == process_id) {
                            return std::addressof(m_infos[i]);
                        }
                    }
                    return nullptr;
                }

                bool Register(os::ProcessId process_id) {
                    /* Allocate an info id. */
                    const auto info_id = m_next_info_id++;

                    /* Check that we have space for the program. */
                    if (m_count >= MaxProgramInfo) {
                        return false;
                    }

                    /* Create the new program info. */
                    m_infos[m_count++] = {
                        .process_id     = process_id,
                        .id             = 0,
                        .socket         = 0,
                        .info_id        = info_id,
                        .started        = false,
                        .jit_debug      = false,
                        .launched_by_cs = false,
                    };

                    return true;
                }

                void Unregister(os::ProcessId process_id) {
                    /* Unregister the program, if it's registered. */
                    for (auto i = 0; i < m_count; ++i) {
                        if (m_infos[i].process_id == process_id) {
                            /* Ensure that the valid program infos remain in bounds. */
                            std::memcpy(m_infos + i, m_infos + i + 1, (m_count - (i + 1)) * sizeof(*m_infos));

                            /* Note that we now have one fewer program info. */
                            --m_count;

                            break;
                        }
                    }
                }

                void SetStarted(os::ProcessId process_id) {
                    /* Start the program. */
                    for (auto i = 0; i < m_count; ++i) {
                        if (m_infos[i].process_id == process_id) {
                            m_infos[i].started = true;
                            break;
                        }
                    }
                }

                void SetJitDebug(os::ProcessId process_id) {
                    /* Set the program as jit debug. */
                    for (auto i = 0; i < m_count; ++i) {
                        if (m_infos[i].process_id == process_id) {
                            m_infos[i].jit_debug = true;
                            break;
                        }
                    }
                }
        };

        alignas(os::ThreadStackAlignment) constinit u8 g_thread_stack[os::MemoryPageSize];
        constinit os::ThreadType g_thread;

        constinit ProcessEventHandler g_common_start_handler;
        constinit ProcessEventHandler g_common_exit_handler;
        constinit ProcessEventHandler g_common_jit_debug_handler;

        constinit SocketInfoManager g_socket_info_manager;
        constinit ProgramInfoManager g_program_info_manager;

        constinit os::SdkMutex g_manager_mutex;

        void ProcessExitEvent(const pm::ProcessEventInfo &event_info) {
            /* Unregister the target environment definition. */
            htc::tenv::UnregisterDefinitionFilePath(event_info.process_id);

            /* Unregister program info. */
            ProgramInfo program_info;
            bool found = false;
            {
                std::scoped_lock lk(g_manager_mutex);

                if (const ProgramInfo *pi = g_program_info_manager.Find(event_info.process_id); pi != nullptr) {
                    program_info = *pi;
                    found = true;

                    g_program_info_manager.Unregister(event_info.process_id);
                }
            }

            /* If we found the program, handle callbacks. */
            if (found) {
                /* Invoke the common exit handler. */
                if (program_info.launched_by_cs) {
                    g_common_exit_handler(program_info.id, program_info.socket, program_info.process_id);
                }

                /* Notify the process event. */
                if (program_info.started) {
                    std::scoped_lock lk(g_manager_mutex);

                    g_socket_info_manager.InvokeHandler(g_common_exit_handler, program_info.process_id);
                }
            }
        }

        void ProcessStartedEvent(const pm::ProcessEventInfo &event_info) {
            /* Start the program (registering it, if needed). */
            {
                std::scoped_lock lk(g_manager_mutex);

                if (g_program_info_manager.Find(event_info.process_id) == nullptr) {
                    AMS_ABORT_UNLESS(g_program_info_manager.Register(event_info.process_id));
                }

                g_program_info_manager.SetStarted(event_info.process_id);
            }

            /* Handle callbacks. */
            {
                std::scoped_lock lk(g_manager_mutex);

                g_socket_info_manager.InvokeHandler(g_common_start_handler, event_info.process_id);
            }
        }

        void ProcessExceptionEvent(const pm::ProcessEventInfo &event_info) {
            /* Find the program info. */
            ProgramInfo program_info;
            bool found = false;
            {
                std::scoped_lock lk(g_manager_mutex);

                if (const ProgramInfo *pi = g_program_info_manager.Find(event_info.process_id); pi != nullptr) {
                    program_info = *pi;
                    found = true;
                }

                /* Set the program as jit debug. */
                g_program_info_manager.SetJitDebug(event_info.process_id);
            }

            /* If we found the program, handle callbacks. */
            if (found) {
                /* Invoke the common exception handler. */
                if (program_info.launched_by_cs) {
                    g_common_jit_debug_handler(program_info.id, program_info.socket, program_info.process_id);
                }

                /* Notify the process event. */
                if (program_info.started) {
                    std::scoped_lock lk(g_manager_mutex);

                    g_socket_info_manager.InvokeHandler(g_common_jit_debug_handler, program_info.process_id);
                }
            }
        }

        void EventHandlerThread(void *) {
            /* Get event observer. */
            pgl::EventObserver observer;
            R_ABORT_UNLESS(pgl::GetEventObserver(std::addressof(observer)));

            /* Get the observer's event. */
            os::SystemEventType shell_event;
            R_ABORT_UNLESS(observer.GetSystemEvent(std::addressof(shell_event)));

            /* Loop handling events. */
            while (true) {
                /* Wait for an event to come in. */
                os::WaitSystemEvent(std::addressof(shell_event));

                /* Loop processing event infos. */
                while (true) {
                    /* Get the next event info. */
                    pm::ProcessEventInfo event_info;
                    if (R_FAILED(observer.GetProcessEventInfo(std::addressof(event_info)))) {
                        break;
                    }

                    /* Process the event. */
                    switch (event_info.GetProcessEvent()) {
                        case pm::ProcessEvent::Exited:
                            ProcessExitEvent(event_info);
                            break;
                        case pm::ProcessEvent::Started:
                            ProcessStartedEvent(event_info);
                            break;
                        case pm::ProcessEvent::Exception:
                            ProcessExceptionEvent(event_info);
                            break;
                        default:
                            break;
                    }
                }
            }
        }

        void StartEventHandlerThread() {
            /* Create the handler thread. */
            R_ABORT_UNLESS(os::CreateThread(std::addressof(g_thread), EventHandlerThread, nullptr, g_thread_stack, sizeof(g_thread_stack), AMS_GET_SYSTEM_THREAD_PRIORITY(scs, ShellEventHandler)));

            /* Set the handler thread's name. */
            os::SetThreadNamePointer(std::addressof(g_thread), AMS_GET_SYSTEM_THREAD_NAME(scs, ShellEventHandler));

            /* Start the handler thread. */
            os::StartThread(std::addressof(g_thread));
        }

        Result PrepareToLaunchProgram(ncm::ProgramId program_id, const void *args, size_t args_size) {
            /* Set the arguments. */
            R_TRY_CATCH(ldr::SetProgramArgument(program_id, args, args_size)) {
                R_CATCH(ldr::ResultArgumentCountOverflow) {
                    /* There are too many arguments already registered. Flush the arguments queue. */
                    R_TRY(ldr::FlushArguments());

                    /* Try again. */
                    R_TRY(ldr::SetProgramArgument(program_id, args, args_size));
                }
            } R_END_TRY_CATCH;

            return ResultSuccess();
        }

        void FlushProgramArgument(ncm::ProgramId program_id) {
            /* Ensure there are no arguments for the program. */
            ldr::SetProgramArgument(program_id, "", 1);
        }

    }

    void InitializeShell() {
        /* Initialize our managers. */
        g_socket_info_manager.Initialize();
        g_program_info_manager.Initialize();

        /* Start our event handler. */
        StartEventHandlerThread();
    }

    void RegisterCommonProcessEventHandler(ProcessEventHandler on_start, ProcessEventHandler on_exit, ProcessEventHandler on_jit_debug) {
        g_common_start_handler     = on_start;
        g_common_exit_handler      = on_exit;
        g_common_jit_debug_handler = on_jit_debug;
    }

    Result RegisterSocket(s32 socket, u64 id) {
        /* Acquire exclusive access to the socket info manager. */
        std::scoped_lock lk(g_manager_mutex);

        /* Register the socket. */
        return g_socket_info_manager.Register(socket, id);
    }

    void UnregisterSocket(s32 socket) {
        /* Acquire exclusive access to the socket info manager. */
        std::scoped_lock lk(g_manager_mutex);

        /* Unregister the socket. */
        return g_socket_info_manager.Unregister(socket);
    }

    Result LaunchProgram(os::ProcessId *out, const ncm::ProgramLocation &loc, const void *args, size_t args_size, u32 process_flags) {
        /* Set up the arguments. */
        PrepareToLaunchProgram(loc.program_id, args, args_size);

        /* Ensure arguments are managed correctly. */
        ON_SCOPE_EXIT { FlushProgramArgument(loc.program_id); };

        /* Launch the program. */
        R_TRY(pgl::LaunchProgram(out, loc, process_flags | pm::LaunchFlags_SignalOnExit, 0));

        return ResultSuccess();
    }

    Result SubscribeProcessEvent(s32 socket, bool is_register, u64 id);

}
