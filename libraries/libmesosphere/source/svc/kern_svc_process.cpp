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

        Result GetProcessId(u64 *out_process_id, ams::svc::Handle handle) {
            /* Get the object from the handle table. */
            KScopedAutoObject obj = GetCurrentProcess().GetHandleTable().GetObject<KAutoObject>(handle);
            R_UNLESS(obj.IsNotNull(), svc::ResultInvalidHandle());

            /* Get the process from the object. */
            KProcess *process = nullptr;
            if (obj->IsDerivedFrom(KProcess::GetStaticTypeObj())) {
                /* The object is a process, so we can use it directly. */
                process = reinterpret_cast<KProcess *>(obj.GetPointerUnsafe());
            } else if (obj->IsDerivedFrom(KThread::GetStaticTypeObj())) {
                /* The object is a thread, so we want to use its parent. */
                process = reinterpret_cast<KThread *>(obj.GetPointerUnsafe())->GetOwnerProcess();
            } else if (obj->IsDerivedFrom(KDebug::GetStaticTypeObj())) {
                /* The object is a debug, so we want to use the process it's attached to. */
                MESOSPHERE_UNIMPLEMENTED();
            }

            /* Make sure the target process exists. */
            R_UNLESS(process != nullptr, svc::ResultInvalidHandle());

            /* Get the process id. */
            *out_process_id = process->GetId();
            return ResultSuccess();
        }



    }

    /* =============================    64 ABI    ============================= */

    void ExitProcess64() {
        MESOSPHERE_PANIC("Stubbed SvcExitProcess64 was called.");
    }

    Result GetProcessId64(uint64_t *out_process_id, ams::svc::Handle process_handle) {
        return GetProcessId(out_process_id, process_handle);
    }

    Result GetProcessList64(int32_t *out_num_processes, KUserPointer<uint64_t *> out_process_ids, int32_t max_out_count) {
        MESOSPHERE_PANIC("Stubbed SvcGetProcessList64 was called.");
    }

    Result CreateProcess64(ams::svc::Handle *out_handle, KUserPointer<const ams::svc::lp64::CreateProcessParameter *> parameters, KUserPointer<const uint32_t *> caps, int32_t num_caps) {
        MESOSPHERE_PANIC("Stubbed SvcCreateProcess64 was called.");
    }

    Result StartProcess64(ams::svc::Handle process_handle, int32_t priority, int32_t core_id, uint64_t main_thread_stack_size) {
        MESOSPHERE_PANIC("Stubbed SvcStartProcess64 was called.");
    }

    Result TerminateProcess64(ams::svc::Handle process_handle) {
        MESOSPHERE_PANIC("Stubbed SvcTerminateProcess64 was called.");
    }

    Result GetProcessInfo64(int64_t *out_info, ams::svc::Handle process_handle, ams::svc::ProcessInfoType info_type) {
        MESOSPHERE_PANIC("Stubbed SvcGetProcessInfo64 was called.");
    }

    /* ============================= 64From32 ABI ============================= */

    void ExitProcess64From32() {
        MESOSPHERE_PANIC("Stubbed SvcExitProcess64From32 was called.");
    }

    Result GetProcessId64From32(uint64_t *out_process_id, ams::svc::Handle process_handle) {
        return GetProcessId(out_process_id, process_handle);
    }

    Result GetProcessList64From32(int32_t *out_num_processes, KUserPointer<uint64_t *> out_process_ids, int32_t max_out_count) {
        MESOSPHERE_PANIC("Stubbed SvcGetProcessList64From32 was called.");
    }

    Result CreateProcess64From32(ams::svc::Handle *out_handle, KUserPointer<const ams::svc::ilp32::CreateProcessParameter *> parameters, KUserPointer<const uint32_t *> caps, int32_t num_caps) {
        MESOSPHERE_PANIC("Stubbed SvcCreateProcess64From32 was called.");
    }

    Result StartProcess64From32(ams::svc::Handle process_handle, int32_t priority, int32_t core_id, uint64_t main_thread_stack_size) {
        MESOSPHERE_PANIC("Stubbed SvcStartProcess64From32 was called.");
    }

    Result TerminateProcess64From32(ams::svc::Handle process_handle) {
        MESOSPHERE_PANIC("Stubbed SvcTerminateProcess64From32 was called.");
    }

    Result GetProcessInfo64From32(int64_t *out_info, ams::svc::Handle process_handle, ams::svc::ProcessInfoType info_type) {
        MESOSPHERE_PANIC("Stubbed SvcGetProcessInfo64From32 was called.");
    }

}
