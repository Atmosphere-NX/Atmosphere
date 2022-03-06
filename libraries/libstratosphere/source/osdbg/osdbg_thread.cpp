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
    #include "impl/osdbg_thread_info.generic.hpp"
#endif

namespace ams::osdbg {

    Result InitializeThreadInfo(ThreadInfo *thread_info, os::NativeHandle debug_handle, const osdbg::DebugInfoCreateProcess *create_process, const osdbg::DebugInfoCreateThread *create_thread) {
        /* Set basic fields. */
        thread_info->_thread_type               = nullptr;
        thread_info->_thread_type_type          = ThreadTypeType_Unknown;
        thread_info->_debug_handle              = debug_handle;
        #if defined(ATMOSPHERE_OS_HORIZON)
        thread_info->_debug_info_create_process = *create_process;
        thread_info->_debug_info_create_thread  = *create_thread;
        #else
        AMS_UNUSED(create_process, create_thread);
        #endif

        /* Update the current info. */
        R_RETURN(impl::ThreadInfoImpl::FillWithCurrentInfo(thread_info));
    }

    Result UpdateThreadInfo(ThreadInfo *thread_info) {
        /* Update the current info. */
        R_RETURN(impl::ThreadInfoImpl::FillWithCurrentInfo(thread_info));
    }

    Result GetThreadName(char *dst, const ThreadInfo *thread_info) {
        #if defined(ATMOSPHERE_OS_HORIZON)
        /* Read the name. */
        if (const auto name_pointer = GetThreadNamePointer(thread_info); name_pointer != 0) {
            R_RETURN(svc::ReadDebugProcessMemory(reinterpret_cast<uintptr_t>(dst), thread_info->_debug_handle, name_pointer, os::ThreadNameLengthMax));
        } else {
            /* Special-case libnx threads. */
            if (thread_info->_thread_type_type == ThreadTypeType_Libnx) {
                util::TSNPrintf(dst, os::ThreadNameLengthMax, "libnx Thread_0x%010" PRIx64 "", reinterpret_cast<uintptr_t>(thread_info->_thread_type));
            } else {
                util::TSNPrintf(dst, os::ThreadNameLengthMax, "Thread_0x%010" PRIx64 "", reinterpret_cast<uintptr_t>(thread_info->_thread_type));
            }

            R_SUCCEED();
        }
        #else
        util::TSNPrintf(dst, os::ThreadNameLengthMax, "Thread_0x%010" PRIx64 "", static_cast<u64>(reinterpret_cast<uintptr_t>(thread_info->_thread_type)));
        R_SUCCEED();
        #endif
    }

}
