/*
 * Copyright (c) 2018-2020 Atmosphère-NX
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
            bool _18;
            bool _19;
            bool _1A;
        };

        constexpr inline auto MaxSocketInfo  = 2;
        constexpr inline auto MaxProgramInfo = 64;

        class SocketInfoManager {
            private:
                SocketInfo m_infos[MaxProgramInfo];
                int m_count;
            public:
                constexpr SocketInfoManager() = default;

                void Initialize() {
                    /* Clear our count. */
                    m_count = 0;
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
        };

        alignas(os::ThreadStackAlignment) constinit u8 g_thread_stack[os::MemoryPageSize];
        constinit os::ThreadType g_thread;

        constinit ProcessEventHandler g_common_start_handler;
        constinit ProcessEventHandler g_common_exit_handler;
        constinit ProcessEventHandler g_common_jit_debug_handler;

        constinit SocketInfoManager g_socket_info_manager;
        constinit ProgramInfoManager g_program_info_manager;

        void EventHandlerThread(void *) {
            /* TODO */
            AMS_ABORT("scs::EventHandlerThread");
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
                R_CATCH(ldr::ResultTooManyArguments) {
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

    bool RegisterSocket(s32 socket);
    void UnregisterSocket(s32 socket);

    Result LaunchProgram(os::ProcessId *out, ncm::ProgramId program_id, const void *args, size_t args_size, u32 process_flags) {
        /* Set up the arguments. */
        PrepareToLaunchProgram(program_id, args, args_size);

        /* Ensure arguments are managed correctly. */
        ON_SCOPE_EXIT { FlushProgramArgument(program_id); };

        /* Launch the program. */
        const ncm::ProgramLocation loc = ncm::ProgramLocation::Make(program_id, ncm::StorageId::BuiltInSystem);
        R_TRY(pgl::LaunchProgram(out, loc, process_flags | pm::LaunchFlags_SignalOnExit, 0));

        return ResultSuccess();
    }

    Result SubscribeProcessEvent(s32 socket, bool is_register, u64 id);

}
