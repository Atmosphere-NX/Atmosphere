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

namespace ams::osdbg::impl {

    namespace {

        Result GetThreadTypePointerFromThreadLocalRegion(uintptr_t *out, ThreadInfo *info) {
            /* Detect lp64. */
            const bool is_lp64 = IsLp64(info);

            /* Get the thread local region. */
            const auto *tlr_address = GetTargetThreadLocalRegion(info);

            /* Read the thread local region. */
            ThreadLocalRegionCommon tlr;
            R_TRY(svc::ReadDebugProcessMemory(reinterpret_cast<uintptr_t>(std::addressof(tlr)), info->_debug_handle, reinterpret_cast<uintptr_t>(tlr_address), sizeof(tlr)));

            /* Detect libnx vs nintendo via magic number. */
            if (tlr.libnx.thread_vars.magic == LibnxThreadVars::Magic) {
                info->_thread_type_type = ThreadTypeType_Libnx;
                *out = reinterpret_cast<uintptr_t>(tlr.libnx.thread_vars.thread_ptr);
            } else {
                info->_thread_type_type = ThreadTypeType_Nintendo;
                *out = is_lp64 ? tlr.lp64.p_thread_type : tlr.ilp32.p_thread_type;
            }

            return ResultSuccess();
        }

        Result GetThreadArgumentAndStackPointer(u64 *out_arg, u64 *out_sp, ThreadInfo *info) {
            /* Read the thread context. */
            svc::ThreadContext thread_context;
            R_TRY(svc::GetDebugThreadContext(std::addressof(thread_context), info->_debug_handle, info->_debug_info_create_thread.thread_id, svc::ThreadContextFlag_General | svc::ThreadContextFlag_Control));

            /* Argument is in r0. */
            *out_arg = thread_context.r[0];

            /* Stack pointer varies by architecture. */
            if (Is64BitArch(info)) {
                *out_sp = thread_context.sp;
            } else {
                *out_sp = thread_context.r[13];
            }

            return ResultSuccess();
        }

        void DetectStratosphereThread(ThreadInfo *info) {
            /* Stratosphere threads are initially misdetected as libnx threads. */
            if (info->_thread_type_type != ThreadTypeType_Libnx || info->_thread_type == nullptr) {
                return;
            }

            /* Convert to a parent pointer. */
            os::ThreadType *stratosphere_ptr = util::GetParentPointer<&os::ThreadType::thread_impl_storage>(std::addressof(info->_thread_type->libnx));

            /* Read the magic. */
            u16 magic;
            if (R_FAILED(svc::ReadDebugProcessMemory(reinterpret_cast<uintptr_t>(std::addressof(magic)), info->_debug_handle, reinterpret_cast<uintptr_t>(std::addressof(stratosphere_ptr->magic)), sizeof(magic)))) {
                return;
            }

            /* Check the magic. */
            if (magic == os::ThreadType::Magic) {
                info->_thread_type_type = ThreadTypeType_Stratosphere;
                info->_thread_type      = reinterpret_cast<ThreadTypeCommon *>(stratosphere_ptr);
            }
        }

    }

    void GetTargetThreadType(ThreadInfo *info) {
        /* Ensure we exit with correct state. */
        auto type_guard = SCOPE_GUARD {
            info->_thread_type      = nullptr;
            info->_thread_type_type = ThreadTypeType_Unknown;
        };

        /* Read the thread type pointer. */
        uintptr_t tlr_thread_type;
        if (R_FAILED(GetThreadTypePointerFromThreadLocalRegion(std::addressof(tlr_thread_type), info))) {
            return;
        }

        /* Handle the case where we have a thread type. */
        if (tlr_thread_type != 0) {
            info->_thread_type = reinterpret_cast<ThreadTypeCommon *>(tlr_thread_type);
            DetectStratosphereThread(info);
            type_guard.Cancel();
            return;
        }

        /* Otherwise, the thread is just created, and we should read its context. */
        u64 arg, sp;
        if (R_FAILED(GetThreadArgumentAndStackPointer(std::addressof(arg), std::addressof(sp), info))) {
            return;
        }

        /* We may have been bamboozled into thinking a nintendo thread was a libnx thread, so check that. */
        /* Nintendo threads have argument=ThreadType, libnx threads have argument=ThreadEntryArgs. */
        if (info->_thread_type_type == ThreadTypeType_Nintendo && sp == arg) {
            /* It's a libnx thread, so we should parse the entry args. */
            info->_thread_type_type = ThreadTypeType_Libnx;

            /* Read the entry args. */
            LibnxThreadEntryArgs entry_args;
            if (R_FAILED(svc::ReadDebugProcessMemory(reinterpret_cast<uintptr_t>(std::addressof(entry_args)), info->_debug_handle, arg, sizeof(entry_args)))) {
                return;
            }

            info->_thread_type      = reinterpret_cast<ThreadTypeCommon *>(entry_args.t);
        } else {
            info->_thread_type_type = ThreadTypeType_Nintendo;
            info->_thread_type      = reinterpret_cast<ThreadTypeCommon *>(arg);
        }

        /* If we got the thread type, we don't need to reset our state. */
        if (info->_thread_type != nullptr) {
            type_guard.Cancel();
            DetectStratosphereThread(info);
        }
    }

}
