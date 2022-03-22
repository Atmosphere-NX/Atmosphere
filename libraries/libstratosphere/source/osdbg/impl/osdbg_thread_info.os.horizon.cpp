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
#include "osdbg_thread_info.os.horizon.hpp"
#include "osdbg_thread_type.os.horizon.hpp"
#include "osdbg_thread_local_region.os.horizon.hpp"
#include "../../os/impl/os_thread_manager_impl.os.horizon.hpp"

namespace ams::osdbg::impl {

    namespace {

        s32 ConvertToUserPriority(s32 horizon_priority) {
            return horizon_priority - os::impl::UserThreadPriorityOffset;
        }

        s32 GetCurrentThreadPriorityImpl(const ThreadInfo *info) {
            u64 dummy;
            u32 horizon_priority;
            if (R_FAILED(svc::GetDebugThreadParam(std::addressof(dummy), std::addressof(horizon_priority), info->_debug_handle, info->_debug_info_create_thread.thread_id, svc::DebugThreadParam_Priority))) {
                return info->_base_priority;
            }

            return ConvertToUserPriority(static_cast<s32>(horizon_priority));
        }

        void FillWithCurrentInfoImpl(ThreadInfo *info, const auto &thread_type_impl) {
            /* Set fields. */
            info->_base_priority    = thread_type_impl._base_priority;
            info->_current_priority = GetCurrentThreadPriorityImpl(info);
            info->_stack_size       = thread_type_impl._stack_size;
            info->_stack            = thread_type_impl._stack;
            info->_argument         = thread_type_impl._argument;
            info->_function         = thread_type_impl._thread_function;
            info->_name_pointer     = thread_type_impl._name_pointer;
        }

    }

    Result ThreadInfoHorizonImpl::FillWithCurrentInfo(ThreadInfo *info) {
        /* Detect lp64. */
        const bool is_lp64 = IsLp64(info);

        /* Ensure that we have a thread type. */
        if (info->_thread_type == nullptr) {
            /* Ensure we exit with correct thread type. */
            auto thread_guard = SCOPE_GUARD { info->_thread_type = nullptr; };

            /* Set the target thread type. */
            GetTargetThreadType(info);

            /* If it's still nullptr, we failed to get the thread type. */
            R_UNLESS(info->_thread_type != nullptr, osdbg::ResultCannotGetThreadInfo());

            /* Check that the thread type is valid. */
            R_UNLESS(info->_thread_type_type != ThreadTypeType_Unknown, osdbg::ResultUnsupportedThreadVersion());

            /* We successfully got the thread type. */
            thread_guard.Cancel();
        }

        /* Read and process the thread type. */
        ThreadTypeCommon thread_type;

        switch (info->_thread_type_type) {
            case ThreadTypeType_Nintendo:
                if (is_lp64) {
                    /* Read in the thread type. */
                    R_TRY(svc::ReadDebugProcessMemory(reinterpret_cast<uintptr_t>(std::addressof(thread_type.lp64)), info->_debug_handle, reinterpret_cast<uintptr_t>(info->_thread_type), sizeof(thread_type.lp64)));

                    /* Process different versions. */
                    switch (thread_type.lp64._version) {
                        case 0x0000:
                        case 0xFFFF:
                            FillWithCurrentInfoImpl(info, thread_type.lp64_v0);
                            break;
                        case 0x0001:
                            FillWithCurrentInfoImpl(info, thread_type.lp64);
                            break;
                        default:
                            return osdbg::ResultUnsupportedThreadVersion();
                    }
                } else {
                    /* Read in the thread type. */
                    R_TRY(svc::ReadDebugProcessMemory(reinterpret_cast<uintptr_t>(std::addressof(thread_type.ilp32)), info->_debug_handle, reinterpret_cast<uintptr_t>(info->_thread_type), sizeof(thread_type.ilp32)));

                    /* Process different versions. */
                    switch (thread_type.ilp32._version) {
                        case 0x0000:
                        case 0xFFFF:
                            FillWithCurrentInfoImpl(info, thread_type.ilp32_v0);
                            break;
                        case 0x0001:
                            FillWithCurrentInfoImpl(info, thread_type.ilp32);
                            break;
                        default:
                            return osdbg::ResultUnsupportedThreadVersion();
                    }
                }
                break;
            case ThreadTypeType_Stratosphere:
                {
                    /* Read in the thread type. */
                    R_TRY(svc::ReadDebugProcessMemory(reinterpret_cast<uintptr_t>(std::addressof(thread_type.stratosphere)), info->_debug_handle, reinterpret_cast<uintptr_t>(info->_thread_type), sizeof(thread_type.stratosphere)));

                    /* Set fields. */
                    const auto &thread_type_impl = thread_type.stratosphere;

                    /* Check that our thread version is valid. */
                    R_UNLESS(thread_type_impl.version == 0x0000 || thread_type_impl.version == 0xFFFF, osdbg::ResultUnsupportedThreadVersion());

                    info->_base_priority    = thread_type_impl.base_priority;
                    info->_current_priority = GetCurrentThreadPriorityImpl(info);
                    info->_stack_size       = thread_type_impl.stack_size;
                    info->_stack            = reinterpret_cast<uintptr_t>(thread_type_impl.stack);
                    info->_argument         = reinterpret_cast<uintptr_t>(thread_type_impl.argument);
                    info->_function         = reinterpret_cast<uintptr_t>(thread_type_impl.function);
                    info->_name_pointer     = reinterpret_cast<uintptr_t>(thread_type_impl.name_pointer);
                }
                break;
            case ThreadTypeType_Libnx:
                {
                    /* Read in the thread type. */
                    R_TRY(svc::ReadDebugProcessMemory(reinterpret_cast<uintptr_t>(std::addressof(thread_type.libnx)), info->_debug_handle, reinterpret_cast<uintptr_t>(info->_thread_type), sizeof(thread_type.libnx)));

                    /* Set fields. */
                    const auto &thread_type_impl = thread_type.libnx;

                    /* NOTE: libnx does not store/track base priority anywhere. */
                    info->_base_priority    = -1;
                    info->_current_priority = GetCurrentThreadPriorityImpl(info);
                    if (info->_current_priority != info->_base_priority) {
                        info->_base_priority = info->_current_priority;
                    }
                    info->_stack_size       = thread_type_impl.stack_sz;
                    info->_stack            = reinterpret_cast<uintptr_t>(thread_type_impl.stack_mirror);

                    /* Parse thread entry args. */
                    {
                        LibnxThreadEntryArgs thread_entry_args;

                        if (R_SUCCEEDED(svc::ReadDebugProcessMemory(reinterpret_cast<uintptr_t>(std::addressof(thread_entry_args)), info->_debug_handle, info->_stack + info->_stack_size, sizeof(LibnxThreadEntryArgs)))) {
                            info->_argument = thread_entry_args.arg;
                            info->_function = thread_entry_args.entry;
                        } else {
                            /* Failed to read the argument/function. */
                            info->_argument = 0;
                            info->_function = 0;
                        }
                    }

                    /* Libnx threads don't have names. */
                    info->_name_pointer     = 0;
                }
                break;
            default:
                return osdbg::ResultUnsupportedThreadVersion();
        }

        return ResultSuccess();
    }

}
