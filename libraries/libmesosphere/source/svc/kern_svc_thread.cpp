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
#include <mesosphere.hpp>

namespace ams::kern::svc {

    /* =============================    Common    ============================= */

    namespace {



    }

    /* =============================    64 ABI    ============================= */

    Result CreateThread64(ams::svc::Handle *out_handle, ams::svc::ThreadFunc func, ams::svc::Address arg, ams::svc::Address stack_bottom, int32_t priority, int32_t core_id) {
        MESOSPHERE_PANIC("Stubbed SvcCreateThread64 was called.");
    }

    Result StartThread64(ams::svc::Handle thread_handle) {
        MESOSPHERE_PANIC("Stubbed SvcStartThread64 was called.");
    }

    void ExitThread64() {
        MESOSPHERE_PANIC("Stubbed SvcExitThread64 was called.");
    }

    void SleepThread64(int64_t ns) {
        MESOSPHERE_PANIC("Stubbed SvcSleepThread64 was called.");
    }

    Result GetThreadPriority64(int32_t *out_priority, ams::svc::Handle thread_handle) {
        MESOSPHERE_PANIC("Stubbed SvcGetThreadPriority64 was called.");
    }

    Result SetThreadPriority64(ams::svc::Handle thread_handle, int32_t priority) {
        MESOSPHERE_PANIC("Stubbed SvcSetThreadPriority64 was called.");
    }

    Result GetThreadCoreMask64(int32_t *out_core_id, uint64_t *out_affinity_mask, ams::svc::Handle thread_handle) {
        MESOSPHERE_PANIC("Stubbed SvcGetThreadCoreMask64 was called.");
    }

    Result SetThreadCoreMask64(ams::svc::Handle thread_handle, int32_t core_id, uint64_t affinity_mask) {
        MESOSPHERE_PANIC("Stubbed SvcSetThreadCoreMask64 was called.");
    }

    Result GetThreadId64(uint64_t *out_thread_id, ams::svc::Handle thread_handle) {
        MESOSPHERE_PANIC("Stubbed SvcGetThreadId64 was called.");
    }

    Result GetDebugFutureThreadInfo64(ams::svc::lp64::LastThreadContext *out_context, uint64_t *thread_id, ams::svc::Handle debug_handle, int64_t ns) {
        MESOSPHERE_PANIC("Stubbed SvcGetDebugFutureThreadInfo64 was called.");
    }

    Result GetLastThreadInfo64(ams::svc::lp64::LastThreadContext *out_context, ams::svc::Address *out_tls_address, uint32_t *out_flags) {
        MESOSPHERE_PANIC("Stubbed SvcGetLastThreadInfo64 was called.");
    }

    Result GetThreadContext364(KUserPointer<ams::svc::ThreadContext *> out_context, ams::svc::Handle thread_handle) {
        MESOSPHERE_PANIC("Stubbed SvcGetThreadContext364 was called.");
    }

    Result GetThreadList64(int32_t *out_num_threads, KUserPointer<uint64_t *> out_thread_ids, int32_t max_out_count, ams::svc::Handle debug_handle) {
        MESOSPHERE_PANIC("Stubbed SvcGetThreadList64 was called.");
    }

    /* ============================= 64From32 ABI ============================= */

    Result CreateThread64From32(ams::svc::Handle *out_handle, ams::svc::ThreadFunc func, ams::svc::Address arg, ams::svc::Address stack_bottom, int32_t priority, int32_t core_id) {
        MESOSPHERE_PANIC("Stubbed SvcCreateThread64From32 was called.");
    }

    Result StartThread64From32(ams::svc::Handle thread_handle) {
        MESOSPHERE_PANIC("Stubbed SvcStartThread64From32 was called.");
    }

    void ExitThread64From32() {
        MESOSPHERE_PANIC("Stubbed SvcExitThread64From32 was called.");
    }

    void SleepThread64From32(int64_t ns) {
        MESOSPHERE_PANIC("Stubbed SvcSleepThread64From32 was called.");
    }

    Result GetThreadPriority64From32(int32_t *out_priority, ams::svc::Handle thread_handle) {
        MESOSPHERE_PANIC("Stubbed SvcGetThreadPriority64From32 was called.");
    }

    Result SetThreadPriority64From32(ams::svc::Handle thread_handle, int32_t priority) {
        MESOSPHERE_PANIC("Stubbed SvcSetThreadPriority64From32 was called.");
    }

    Result GetThreadCoreMask64From32(int32_t *out_core_id, uint64_t *out_affinity_mask, ams::svc::Handle thread_handle) {
        MESOSPHERE_PANIC("Stubbed SvcGetThreadCoreMask64From32 was called.");
    }

    Result SetThreadCoreMask64From32(ams::svc::Handle thread_handle, int32_t core_id, uint64_t affinity_mask) {
        MESOSPHERE_PANIC("Stubbed SvcSetThreadCoreMask64From32 was called.");
    }

    Result GetThreadId64From32(uint64_t *out_thread_id, ams::svc::Handle thread_handle) {
        MESOSPHERE_PANIC("Stubbed SvcGetThreadId64From32 was called.");
    }

    Result GetDebugFutureThreadInfo64From32(ams::svc::ilp32::LastThreadContext *out_context, uint64_t *thread_id, ams::svc::Handle debug_handle, int64_t ns) {
        MESOSPHERE_PANIC("Stubbed SvcGetDebugFutureThreadInfo64From32 was called.");
    }

    Result GetLastThreadInfo64From32(ams::svc::ilp32::LastThreadContext *out_context, ams::svc::Address *out_tls_address, uint32_t *out_flags) {
        MESOSPHERE_PANIC("Stubbed SvcGetLastThreadInfo64From32 was called.");
    }

    Result GetThreadContext364From32(KUserPointer<ams::svc::ThreadContext *> out_context, ams::svc::Handle thread_handle) {
        MESOSPHERE_PANIC("Stubbed SvcGetThreadContext364From32 was called.");
    }

    Result GetThreadList64From32(int32_t *out_num_threads, KUserPointer<uint64_t *> out_thread_ids, int32_t max_out_count, ams::svc::Handle debug_handle) {
        MESOSPHERE_PANIC("Stubbed SvcGetThreadList64From32 was called.");
    }

}
