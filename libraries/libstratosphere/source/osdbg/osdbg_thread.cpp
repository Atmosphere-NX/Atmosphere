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

#if defined(ATMOSPHERE_OS_HORIZON)
    #include "impl/osdbg_thread_info.os.horizon.hpp"
#else
    #error "Unknown OS for ams::osdbg::ThreadInfo"
#endif

namespace ams::osdbg {

    Result InitializeThreadInfo(ThreadInfo *thread_info, os::NativeHandle debug_handle, const svc::DebugInfoCreateProcess *create_process, const svc::DebugInfoCreateThread *create_thread) {
        /* Set basic fields. */
        thread_info->_thread_type               = nullptr;
        thread_info->_thread_type_type          = ThreadTypeType_Unknown;
        thread_info->_debug_handle              = debug_handle;
        thread_info->_debug_info_create_process = *create_process;
        thread_info->_debug_info_create_thread  = *create_thread;

        /* Update the current info. */
        return impl::ThreadInfoImpl::FillWithCurrentInfo(thread_info);
    }

    Result UpdateThreadInfo(ThreadInfo *thread_info) {
        /* Update the current info. */
        return impl::ThreadInfoImpl::FillWithCurrentInfo(thread_info);
    }

    Result GetThreadName(char *dst, const ThreadInfo *thread_info) {
        /* Get the name pointer. */
        const auto name_pointer = GetThreadNamePointer(thread_info);

        /* Read the name. */
        if (name_pointer != 0) {
            return svc::ReadDebugProcessMemory(reinterpret_cast<uintptr_t>(dst), thread_info->_debug_handle, name_pointer, os::ThreadNameLengthMax);
        } else {
            /* Special-case libnx threads. */
            if (thread_info->_thread_type_type == ThreadTypeType_Libnx) {
                util::TSNPrintf(dst, os::ThreadNameLengthMax, "libnx Thread_0x%010lx", reinterpret_cast<uintptr_t>(thread_info->_thread_type));
            } else {
                util::TSNPrintf(dst, os::ThreadNameLengthMax, "Thread_0x%010lx", reinterpret_cast<uintptr_t>(thread_info->_thread_type));
            }

            return ResultSuccess();
        }
    }

}
