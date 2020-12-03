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

#if defined(ATMOSPHERE_BOARD_NINTENDO_NX)

    namespace ams::svc {

        #if defined(ATMOSPHERE_ARCH_ARM64)

            namespace aarch64::lp64 {

                ALWAYS_INLINE Result SetHeapSize(::ams::svc::Address *out_address, ::ams::svc::Size size) {
                    return ::svcSetHeapSize(reinterpret_cast<void **>(out_address), size);
                }

                ALWAYS_INLINE Result SetMemoryPermission(::ams::svc::Address address, ::ams::svc::Size size, ::ams::svc::MemoryPermission perm) {
                    return ::svcSetMemoryPermission(reinterpret_cast<void *>(static_cast<uintptr_t>(address)), size, static_cast<u32>(perm));
                }

                ALWAYS_INLINE Result SetMemoryAttribute(::ams::svc::Address address, ::ams::svc::Size size, uint32_t mask, uint32_t attr) {
                    return ::svcSetMemoryAttribute(reinterpret_cast<void *>(static_cast<uintptr_t>(address)), size, mask, attr);
                }

                ALWAYS_INLINE Result MapMemory(::ams::svc::Address dst_address, ::ams::svc::Address src_address, ::ams::svc::Size size) {
                    return ::svcMapMemory(reinterpret_cast<void *>(static_cast<uintptr_t>(dst_address)), reinterpret_cast<void *>(static_cast<uintptr_t>(src_address)), size);
                }

                ALWAYS_INLINE Result UnmapMemory(::ams::svc::Address dst_address, ::ams::svc::Address src_address, ::ams::svc::Size size) {
                    return ::svcUnmapMemory(reinterpret_cast<void *>(static_cast<uintptr_t>(dst_address)), reinterpret_cast<void *>(static_cast<uintptr_t>(src_address)), size);
                }

                ALWAYS_INLINE Result QueryMemory(::ams::svc::UserPointer< ::ams::svc::lp64::MemoryInfo *> out_memory_info, ::ams::svc::PageInfo *out_page_info, ::ams::svc::Address address) {
                    return ::svcQueryMemory(reinterpret_cast<::MemoryInfo *>(out_memory_info.GetPointerUnsafe()), reinterpret_cast<u32 *>(out_page_info), address);
                }

                ALWAYS_INLINE void ExitProcess() {
                    return ::svcExitProcess();
                }

                ALWAYS_INLINE Result CreateThread(::ams::svc::Handle *out_handle, ::ams::svc::ThreadFunc func, ::ams::svc::Address arg, ::ams::svc::Address stack_bottom, int32_t priority, int32_t core_id) {
                    return ::svcCreateThread(out_handle, reinterpret_cast<void *>(static_cast<uintptr_t>(func)), reinterpret_cast<void *>(static_cast<uintptr_t>(arg)), reinterpret_cast<void *>(static_cast<uintptr_t>(stack_bottom)), priority, core_id);
                }

                ALWAYS_INLINE Result StartThread(::ams::svc::Handle thread_handle) {
                    return ::svcStartThread(thread_handle);
                }

                ALWAYS_INLINE void ExitThread() {
                    return ::svcExitThread();
                }

                ALWAYS_INLINE void SleepThread(int64_t ns) {
                    return ::svcSleepThread(ns);
                }

                ALWAYS_INLINE Result GetThreadPriority(int32_t *out_priority, ::ams::svc::Handle thread_handle) {
                    return ::svcGetThreadPriority(out_priority, thread_handle);
                }

                ALWAYS_INLINE Result SetThreadPriority(::ams::svc::Handle thread_handle, int32_t priority) {
                    return ::svcSetThreadPriority(thread_handle, priority);
                }

                ALWAYS_INLINE Result GetThreadCoreMask(int32_t *out_core_id, uint64_t *out_affinity_mask, ::ams::svc::Handle thread_handle) {
                    return ::svcGetThreadCoreMask(out_core_id, out_affinity_mask, thread_handle);
                }

                ALWAYS_INLINE Result SetThreadCoreMask(::ams::svc::Handle thread_handle, int32_t core_id, uint64_t affinity_mask) {
                    return ::svcSetThreadCoreMask(thread_handle, core_id, affinity_mask);
                }

                ALWAYS_INLINE int32_t GetCurrentProcessorNumber() {
                    return ::svcGetCurrentProcessorNumber();
                }

                ALWAYS_INLINE Result SignalEvent(::ams::svc::Handle event_handle) {
                    return ::svcSignalEvent(event_handle);
                }

                ALWAYS_INLINE Result ClearEvent(::ams::svc::Handle event_handle) {
                    return ::svcClearEvent(event_handle);
                }

                ALWAYS_INLINE Result MapSharedMemory(::ams::svc::Handle shmem_handle, ::ams::svc::Address address, ::ams::svc::Size size, ::ams::svc::MemoryPermission map_perm) {
                    return ::svcMapSharedMemory(shmem_handle, reinterpret_cast<void *>(static_cast<uintptr_t>(address)), size, static_cast<u32>(map_perm));
                }

                ALWAYS_INLINE Result UnmapSharedMemory(::ams::svc::Handle shmem_handle, ::ams::svc::Address address, ::ams::svc::Size size) {
                    return ::svcUnmapSharedMemory(shmem_handle, reinterpret_cast<void *>(static_cast<uintptr_t>(address)), size);
                }

                ALWAYS_INLINE Result CreateTransferMemory(::ams::svc::Handle *out_handle, ::ams::svc::Address address, ::ams::svc::Size size, ::ams::svc::MemoryPermission map_perm) {
                    return ::svcCreateTransferMemory(out_handle, reinterpret_cast<void *>(static_cast<uintptr_t>(address)), size, static_cast<u32>(map_perm));
                }

                ALWAYS_INLINE Result CloseHandle(::ams::svc::Handle handle) {
                    return ::svcCloseHandle(handle);
                }

                ALWAYS_INLINE Result ResetSignal(::ams::svc::Handle handle) {
                    return ::svcResetSignal(handle);
                }

                ALWAYS_INLINE Result WaitSynchronization(int32_t *out_index, ::ams::svc::UserPointer<const ::ams::svc::Handle *> handles, int32_t num_handles, int64_t timeout_ns) {
                    return ::svcWaitSynchronization(out_index, handles.GetPointerUnsafe(), num_handles, timeout_ns);
                }

                ALWAYS_INLINE Result CancelSynchronization(::ams::svc::Handle handle) {
                    return ::svcCancelSynchronization(handle);
                }

                ALWAYS_INLINE Result ArbitrateLock(::ams::svc::Handle thread_handle, ::ams::svc::Address address, uint32_t tag) {
                    return ::svcArbitrateLock(thread_handle, reinterpret_cast<u32 *>(static_cast<uintptr_t>(address)), tag);
                }

                ALWAYS_INLINE Result ArbitrateUnlock(::ams::svc::Address address) {
                    return ::svcArbitrateUnlock(reinterpret_cast<u32 *>(static_cast<uintptr_t>(address)));
                }

                ALWAYS_INLINE Result WaitProcessWideKeyAtomic(::ams::svc::Address address, ::ams::svc::Address cv_key, uint32_t tag, int64_t timeout_ns) {
                    return ::svcWaitProcessWideKeyAtomic(reinterpret_cast<u32 *>(static_cast<uintptr_t>(address)), reinterpret_cast<u32 *>(static_cast<uintptr_t>(cv_key)), tag, timeout_ns);
                }

                ALWAYS_INLINE void SignalProcessWideKey(::ams::svc::Address cv_key, int32_t count) {
                    return ::svcSignalProcessWideKey(reinterpret_cast<u32 *>(static_cast<uintptr_t>(cv_key)), count);
                }

                ALWAYS_INLINE int64_t GetSystemTick() {
                    return ::svcGetSystemTick();
                }

                ALWAYS_INLINE Result ConnectToNamedPort(::ams::svc::Handle *out_handle, ::ams::svc::UserPointer<const char *> name) {
                    return ::svcConnectToNamedPort(out_handle, name.GetPointerUnsafe());
                }

                ALWAYS_INLINE Result SendSyncRequestLight(::ams::svc::Handle session_handle) {
                    return ::svcSendSyncRequestLight(session_handle);
                }

                ALWAYS_INLINE Result SendSyncRequest(::ams::svc::Handle session_handle) {
                    return ::svcSendSyncRequest(session_handle);
                }

                ALWAYS_INLINE Result SendSyncRequestWithUserBuffer(::ams::svc::Address message_buffer, ::ams::svc::Size message_buffer_size, ::ams::svc::Handle session_handle) {
                    return ::svcSendSyncRequestWithUserBuffer(reinterpret_cast<void *>(static_cast<uintptr_t>(message_buffer)), message_buffer_size, session_handle);
                }

                ALWAYS_INLINE Result SendAsyncRequestWithUserBuffer(::ams::svc::Handle *out_event_handle, ::ams::svc::Address message_buffer, ::ams::svc::Size message_buffer_size, ::ams::svc::Handle session_handle) {
                    return ::svcSendAsyncRequestWithUserBuffer(out_event_handle, reinterpret_cast<void *>(static_cast<uintptr_t>(message_buffer)), message_buffer_size, session_handle);
                }

                ALWAYS_INLINE Result GetProcessId(uint64_t *out_process_id, ::ams::svc::Handle process_handle) {
                    return ::svcGetProcessId(out_process_id, process_handle);
                }

                ALWAYS_INLINE Result GetThreadId(uint64_t *out_thread_id, ::ams::svc::Handle thread_handle) {
                    return ::svcGetThreadId(out_thread_id, thread_handle);
                }

                ALWAYS_INLINE void Break(::ams::svc::BreakReason break_reason, ::ams::svc::Address arg, ::ams::svc::Size size) {
                    ::svcBreak(break_reason, arg, size);
                }

                ALWAYS_INLINE Result OutputDebugString(::ams::svc::UserPointer<const char *> debug_str, ::ams::svc::Size len) {
                    return ::svcOutputDebugString(debug_str.GetPointerUnsafe(), len);
                }

                ALWAYS_INLINE void ReturnFromException(::ams::Result result) {
                    return ::svcReturnFromException(result.GetValue());
                }

                ALWAYS_INLINE Result GetInfo(uint64_t *out, ::ams::svc::InfoType info_type, ::ams::svc::Handle handle, uint64_t info_subtype) {
                    return ::svcGetInfo(out, static_cast<u32>(info_type), handle, info_subtype);
                }

                ALWAYS_INLINE void FlushEntireDataCache() {
                    return ::svcFlushEntireDataCache();
                }

                ALWAYS_INLINE Result FlushDataCache(::ams::svc::Address address, ::ams::svc::Size size) {
                    return ::svcFlushDataCache(reinterpret_cast<void *>(static_cast<uintptr_t>(address)), size);
                }

                ALWAYS_INLINE Result MapPhysicalMemory(::ams::svc::Address address, ::ams::svc::Size size) {
                    return ::svcMapPhysicalMemory(reinterpret_cast<void *>(static_cast<uintptr_t>(address)), size);
                }

                ALWAYS_INLINE Result UnmapPhysicalMemory(::ams::svc::Address address, ::ams::svc::Size size) {
                    return ::svcUnmapPhysicalMemory(reinterpret_cast<void *>(static_cast<uintptr_t>(address)), size);
                }

                ALWAYS_INLINE Result GetDebugFutureThreadInfo(::ams::svc::lp64::LastThreadContext *out_context, uint64_t *thread_id, ::ams::svc::Handle debug_handle, int64_t ns) {
                    return ::svcGetDebugFutureThreadInfo(reinterpret_cast<::LastThreadContext *>(out_context), thread_id, debug_handle, ns);
                }

                ALWAYS_INLINE Result GetLastThreadInfo(::ams::svc::lp64::LastThreadContext *out_context, ::ams::svc::Address *out_tls_address, uint32_t *out_flags) {
                    return ::svcGetLastThreadInfo(reinterpret_cast<::LastThreadContext *>(out_context), reinterpret_cast<u64 *>(out_tls_address), out_flags);
                }

                ALWAYS_INLINE Result GetResourceLimitLimitValue(int64_t *out_limit_value, ::ams::svc::Handle resource_limit_handle, ::ams::svc::LimitableResource which) {
                    return ::svcGetResourceLimitLimitValue(out_limit_value, resource_limit_handle, static_cast<::LimitableResource>(which));
                }

                ALWAYS_INLINE Result GetResourceLimitCurrentValue(int64_t *out_current_value, ::ams::svc::Handle resource_limit_handle, ::ams::svc::LimitableResource which) {
                    return ::svcGetResourceLimitCurrentValue(out_current_value, resource_limit_handle, static_cast<::LimitableResource>(which));
                }

                ALWAYS_INLINE Result SetThreadActivity(::ams::svc::Handle thread_handle, ::ams::svc::ThreadActivity thread_activity) {
                    return ::svcSetThreadActivity(thread_handle, static_cast<::ThreadActivity>(thread_activity));
                }

                ALWAYS_INLINE Result GetThreadContext3(::ams::svc::UserPointer< ::ams::svc::ThreadContext *> out_context, ::ams::svc::Handle thread_handle) {
                    return ::svcGetThreadContext3(reinterpret_cast<::ThreadContext *>(out_context.GetPointerUnsafe()), thread_handle);
                }

                ALWAYS_INLINE Result WaitForAddress(::ams::svc::Address address, ::ams::svc::ArbitrationType arb_type, int32_t value, int64_t timeout_ns) {
                    return ::svcWaitForAddress(reinterpret_cast<void *>(static_cast<uintptr_t>(address)), arb_type, value, timeout_ns);
                }

                ALWAYS_INLINE Result SignalToAddress(::ams::svc::Address address, ::ams::svc::SignalType signal_type, int32_t value, int32_t count) {
                    return ::svcSignalToAddress(reinterpret_cast<void *>(static_cast<uintptr_t>(address)), signal_type, value, count);
                }

                ALWAYS_INLINE void SynchronizePreemptionState() {
                    return ::svcSynchronizePreemptionState();
                }

                ALWAYS_INLINE Result GetResourceLimitPeakValue(int64_t *out_peak_value, ::ams::svc::Handle resource_limit_handle, ::ams::svc::LimitableResource which) {
                    return ::svcGetResourceLimitPeakValue(out_peak_value, resource_limit_handle, static_cast<::LimitableResource>(which));
                }

                ALWAYS_INLINE void KernelDebug(::ams::svc::KernelDebugType kern_debug_type, uint64_t arg0, uint64_t arg1, uint64_t arg2) {
                    return ::svcKernelDebug(kern_debug_type, arg0, arg1, arg2);
                }

                ALWAYS_INLINE void ChangeKernelTraceState(::ams::svc::KernelTraceState kern_trace_state) {
                    return ::svcChangeKernelTraceState(kern_trace_state);
                }

                ALWAYS_INLINE Result CreateSession(::ams::svc::Handle *out_server_session_handle, ::ams::svc::Handle *out_client_session_handle, bool is_light, ::ams::svc::Address name) {
                    return ::svcCreateSession(out_server_session_handle, out_client_session_handle, is_light, name);
                }

                ALWAYS_INLINE Result AcceptSession(::ams::svc::Handle *out_handle, ::ams::svc::Handle port) {
                    return ::svcAcceptSession(out_handle, port);
                }

                ALWAYS_INLINE Result ReplyAndReceiveLight(::ams::svc::Handle handle) {
                    return ::svcReplyAndReceiveLight(handle);
                }

                ALWAYS_INLINE Result ReplyAndReceive(int32_t *out_index, ::ams::svc::UserPointer<const ::ams::svc::Handle *> handles, int32_t num_handles, ::ams::svc::Handle reply_target, int64_t timeout_ns) {
                    return ::svcReplyAndReceive(out_index, handles.GetPointerUnsafe(), num_handles, reply_target, timeout_ns);
                }

                ALWAYS_INLINE Result ReplyAndReceiveWithUserBuffer(int32_t *out_index, ::ams::svc::Address message_buffer, ::ams::svc::Size message_buffer_size, ::ams::svc::UserPointer<const ::ams::svc::Handle *> handles, int32_t num_handles, ::ams::svc::Handle reply_target, int64_t timeout_ns) {
                    return ::svcReplyAndReceiveWithUserBuffer(out_index, reinterpret_cast<void *>(static_cast<uintptr_t>(message_buffer)), message_buffer_size, handles.GetPointerUnsafe(), num_handles, reply_target, timeout_ns);
                }

                ALWAYS_INLINE Result CreateEvent(::ams::svc::Handle *out_write_handle, ::ams::svc::Handle *out_read_handle) {
                    return ::svcCreateEvent(out_write_handle, out_read_handle);
                }

                ALWAYS_INLINE Result MapPhysicalMemoryUnsafe(::ams::svc::Address address, ::ams::svc::Size size) {
                    return ::svcMapPhysicalMemoryUnsafe(reinterpret_cast<void *>(static_cast<uintptr_t>(address)), size);
                }

                ALWAYS_INLINE Result UnmapPhysicalMemoryUnsafe(::ams::svc::Address address, ::ams::svc::Size size) {
                    return ::svcUnmapPhysicalMemoryUnsafe(reinterpret_cast<void *>(static_cast<uintptr_t>(address)), size);
                }

                ALWAYS_INLINE Result SetUnsafeLimit(::ams::svc::Size limit) {
                    return ::svcSetUnsafeLimit(limit);
                }

                ALWAYS_INLINE Result CreateCodeMemory(::ams::svc::Handle *out_handle, ::ams::svc::Address address, ::ams::svc::Size size) {
                    return ::svcCreateCodeMemory(out_handle, reinterpret_cast<void *>(static_cast<uintptr_t>(address)), size);
                }

                ALWAYS_INLINE Result ControlCodeMemory(::ams::svc::Handle code_memory_handle, ::ams::svc::CodeMemoryOperation operation, uint64_t address, uint64_t size, ::ams::svc::MemoryPermission perm) {
                    return ::svcControlCodeMemory(code_memory_handle, static_cast<::CodeMapOperation>(operation), reinterpret_cast<void *>(address), size, static_cast<u32>(perm));
                }

                ALWAYS_INLINE void SleepSystem() {
                    return ::svcSleepSystem();
                }

                ALWAYS_INLINE Result ReadWriteRegister(uint32_t *out_value, ::ams::svc::PhysicalAddress address, uint32_t mask, uint32_t value) {
                    return ::svcReadWriteRegister(out_value, address, mask, value);
                }

                ALWAYS_INLINE Result SetProcessActivity(::ams::svc::Handle process_handle, ::ams::svc::ProcessActivity process_activity) {
                    return ::svcSetProcessActivity(process_handle, static_cast<::ProcessActivity>(process_activity));
                }

                ALWAYS_INLINE Result CreateSharedMemory(::ams::svc::Handle *out_handle, ::ams::svc::Size size, ::ams::svc::MemoryPermission owner_perm, ::ams::svc::MemoryPermission remote_perm) {
                    return ::svcCreateSharedMemory(out_handle, size, static_cast<u32>(owner_perm), static_cast<u32>(remote_perm));
                }

                ALWAYS_INLINE Result MapTransferMemory(::ams::svc::Handle trmem_handle, ::ams::svc::Address address, ::ams::svc::Size size, ::ams::svc::MemoryPermission owner_perm) {
                    return ::svcMapTransferMemory(trmem_handle, reinterpret_cast<void *>(static_cast<uintptr_t>(address)), size, static_cast<u32>(owner_perm));
                }

                ALWAYS_INLINE Result UnmapTransferMemory(::ams::svc::Handle trmem_handle, ::ams::svc::Address address, ::ams::svc::Size size) {
                    return ::svcUnmapTransferMemory(trmem_handle, reinterpret_cast<void *>(static_cast<uintptr_t>(address)), size);
                }

                ALWAYS_INLINE Result CreateInterruptEvent(::ams::svc::Handle *out_read_handle, int32_t interrupt_id, ::ams::svc::InterruptType interrupt_type) {
                    return ::svcCreateInterruptEvent(out_read_handle, interrupt_id, static_cast<u32>(interrupt_type));
                }

                ALWAYS_INLINE Result QueryPhysicalAddress(::ams::svc::lp64::PhysicalMemoryInfo *out_info, ::ams::svc::Address address) {
                    return ::svcQueryPhysicalAddress(reinterpret_cast<::PhysicalMemoryInfo *>(out_info), address);
                }

                ALWAYS_INLINE Result QueryIoMapping(::ams::svc::Address *out_address, ::ams::svc::Size *out_size, ::ams::svc::PhysicalAddress physical_address, ::ams::svc::Size size) {
                    return ::svcQueryIoMapping(reinterpret_cast<u64 *>(out_address), reinterpret_cast<u64 *>(out_size), physical_address, size);
                }

                ALWAYS_INLINE Result LegacyQueryIoMapping(::ams::svc::Address *out_address, ::ams::svc::PhysicalAddress physical_address, ::ams::svc::Size size) {
                    return ::svcLegacyQueryIoMapping(reinterpret_cast<u64 *>(out_address), physical_address, size);
                }

                ALWAYS_INLINE Result CreateDeviceAddressSpace(::ams::svc::Handle *out_handle, uint64_t das_address, uint64_t das_size) {
                    return ::svcCreateDeviceAddressSpace(out_handle, das_address, das_size);
                }

                ALWAYS_INLINE Result AttachDeviceAddressSpace(::ams::svc::DeviceName device_name, ::ams::svc::Handle das_handle) {
                    return ::svcAttachDeviceAddressSpace(static_cast<u64>(device_name), das_handle);
                }

                ALWAYS_INLINE Result DetachDeviceAddressSpace(::ams::svc::DeviceName device_name, ::ams::svc::Handle das_handle) {
                    return ::svcDetachDeviceAddressSpace(static_cast<u64>(device_name), das_handle);
                }

                ALWAYS_INLINE Result MapDeviceAddressSpaceByForce(::ams::svc::Handle das_handle, ::ams::svc::Handle process_handle, uint64_t process_address, ::ams::svc::Size size, uint64_t device_address, ::ams::svc::MemoryPermission device_perm) {
                    return ::svcMapDeviceAddressSpaceByForce(das_handle, process_handle, process_address, size, device_address, static_cast<u32>(device_perm));
                }

                ALWAYS_INLINE Result MapDeviceAddressSpaceAligned(::ams::svc::Handle das_handle, ::ams::svc::Handle process_handle, uint64_t process_address, ::ams::svc::Size size, uint64_t device_address, ::ams::svc::MemoryPermission device_perm) {
                    return ::svcMapDeviceAddressSpaceAligned(das_handle, process_handle, process_address, size, device_address, static_cast<u32>(device_perm));
                }

                ALWAYS_INLINE Result MapDeviceAddressSpace(::ams::svc::Size *out_mapped_size, ::ams::svc::Handle das_handle, ::ams::svc::Handle process_handle, uint64_t process_address, ::ams::svc::Size size, uint64_t device_address, ::ams::svc::MemoryPermission device_perm) {
                    return ::svcMapDeviceAddressSpace(reinterpret_cast<u64 *>(out_mapped_size), das_handle, process_handle, process_address, size, device_address, static_cast<u32>(device_perm));
                }

                ALWAYS_INLINE Result UnmapDeviceAddressSpace(::ams::svc::Handle das_handle, ::ams::svc::Handle process_handle, uint64_t process_address, ::ams::svc::Size size, uint64_t device_address) {
                    return ::svcUnmapDeviceAddressSpace(das_handle, process_handle, process_address, size, device_address);
                }

                ALWAYS_INLINE Result InvalidateProcessDataCache(::ams::svc::Handle process_handle, uint64_t address, uint64_t size) {
                    return ::svcInvalidateProcessDataCache(process_handle, address, size);
                }

                ALWAYS_INLINE Result StoreProcessDataCache(::ams::svc::Handle process_handle, uint64_t address, uint64_t size) {
                    return ::svcStoreProcessDataCache(process_handle, address, size);
                }

                ALWAYS_INLINE Result FlushProcessDataCache(::ams::svc::Handle process_handle, uint64_t address, uint64_t size) {
                    return ::svcFlushProcessDataCache(process_handle, address, size);
                }

                ALWAYS_INLINE Result DebugActiveProcess(::ams::svc::Handle *out_handle, uint64_t process_id) {
                    return ::svcDebugActiveProcess(out_handle, process_id);
                }

                ALWAYS_INLINE Result BreakDebugProcess(::ams::svc::Handle debug_handle) {
                    return ::svcBreakDebugProcess(debug_handle);
                }

                ALWAYS_INLINE Result TerminateDebugProcess(::ams::svc::Handle debug_handle) {
                    return ::svcTerminateDebugProcess(debug_handle);
                }

                ALWAYS_INLINE Result GetDebugEvent(::ams::svc::UserPointer< ::ams::svc::lp64::DebugEventInfo *> out_info, ::ams::svc::Handle debug_handle) {
                    return ::svcGetDebugEvent(out_info.GetPointerUnsafe(), debug_handle);
                }

                ALWAYS_INLINE Result ContinueDebugEvent(::ams::svc::Handle debug_handle, uint32_t flags, ::ams::svc::UserPointer<const uint64_t *> thread_ids, int32_t num_thread_ids) {
                    return ::svcContinueDebugEvent(debug_handle, flags, const_cast<u64 *>(thread_ids.GetPointerUnsafe()), num_thread_ids);
                }

                ALWAYS_INLINE Result GetProcessList(int32_t *out_num_processes, ::ams::svc::UserPointer<uint64_t *> out_process_ids, int32_t max_out_count) {
                    return ::svcGetProcessList(out_num_processes, out_process_ids.GetPointerUnsafe(), max_out_count);
                }

                ALWAYS_INLINE Result GetThreadList(int32_t *out_num_threads, ::ams::svc::UserPointer<uint64_t *> out_thread_ids, int32_t max_out_count, ::ams::svc::Handle debug_handle) {
                    return ::svcGetThreadList(out_num_threads, out_thread_ids.GetPointerUnsafe(), max_out_count, debug_handle);
                }

                ALWAYS_INLINE Result GetDebugThreadContext(::ams::svc::UserPointer< ::ams::svc::ThreadContext *> out_context, ::ams::svc::Handle debug_handle, uint64_t thread_id, uint32_t context_flags) {
                    return ::svcGetDebugThreadContext(reinterpret_cast<::ThreadContext *>(out_context.GetPointerUnsafe()), debug_handle, thread_id, context_flags);
                }

                ALWAYS_INLINE Result SetDebugThreadContext(::ams::svc::Handle debug_handle, uint64_t thread_id, ::ams::svc::UserPointer<const ::ams::svc::ThreadContext *> context, uint32_t context_flags) {
                    return ::svcSetDebugThreadContext(debug_handle, thread_id, reinterpret_cast<const ::ThreadContext *>(context.GetPointerUnsafe()), context_flags);
                }

                ALWAYS_INLINE Result QueryDebugProcessMemory(::ams::svc::UserPointer< ::ams::svc::lp64::MemoryInfo *> out_memory_info, ::ams::svc::PageInfo *out_page_info, ::ams::svc::Handle process_handle, ::ams::svc::Address address) {
                    return ::svcQueryDebugProcessMemory(reinterpret_cast<::MemoryInfo *>(out_memory_info.GetPointerUnsafe()), reinterpret_cast<u32 *>(out_page_info), process_handle, address);
                }

                ALWAYS_INLINE Result ReadDebugProcessMemory(::ams::svc::Address buffer, ::ams::svc::Handle debug_handle, ::ams::svc::Address address, ::ams::svc::Size size) {
                    return ::svcReadDebugProcessMemory(reinterpret_cast<void *>(static_cast<uintptr_t>(buffer)), debug_handle, address, size);
                }

                ALWAYS_INLINE Result WriteDebugProcessMemory(::ams::svc::Handle debug_handle, ::ams::svc::Address buffer, ::ams::svc::Address address, ::ams::svc::Size size) {
                    return ::svcWriteDebugProcessMemory(debug_handle, reinterpret_cast<const void *>(static_cast<uintptr_t>(buffer)), address, size);
                }

                ALWAYS_INLINE Result SetHardwareBreakPoint(::ams::svc::HardwareBreakPointRegisterName name, uint64_t flags, uint64_t value) {
                    return ::svcSetHardwareBreakPoint(static_cast<u32>(name), flags, value);
                }

                ALWAYS_INLINE Result GetDebugThreadParam(uint64_t *out_64, uint32_t *out_32, ::ams::svc::Handle debug_handle, uint64_t thread_id, ::ams::svc::DebugThreadParam param) {
                    return ::svcGetDebugThreadParam(out_64, out_32, debug_handle, thread_id, static_cast<::DebugThreadParam>(param));
                }

                ALWAYS_INLINE Result GetSystemInfo(uint64_t *out, ::ams::svc::SystemInfoType info_type, ::ams::svc::Handle handle, uint64_t info_subtype) {
                    return ::svcGetSystemInfo(out, static_cast<u64>(info_type), handle, info_subtype);
                }

                ALWAYS_INLINE Result CreatePort(::ams::svc::Handle *out_server_handle, ::ams::svc::Handle *out_client_handle, int32_t max_sessions, bool is_light, ::ams::svc::Address name) {
                    return ::svcCreatePort(out_server_handle, out_client_handle, max_sessions, is_light, reinterpret_cast<const char *>(static_cast<uintptr_t>(name)));
                }

                ALWAYS_INLINE Result ManageNamedPort(::ams::svc::Handle *out_server_handle, ::ams::svc::UserPointer<const char *> name, int32_t max_sessions) {
                    return ::svcManageNamedPort(out_server_handle, name.GetPointerUnsafe(), max_sessions);
                }

                ALWAYS_INLINE Result ConnectToPort(::ams::svc::Handle *out_handle, ::ams::svc::Handle port) {
                    return ::svcConnectToPort(out_handle, port);
                }

                ALWAYS_INLINE Result SetProcessMemoryPermission(::ams::svc::Handle process_handle, uint64_t address, uint64_t size, ::ams::svc::MemoryPermission perm) {
                    return ::svcSetProcessMemoryPermission(process_handle, address, size, static_cast<u32>(perm));
                }

                ALWAYS_INLINE Result MapProcessMemory(::ams::svc::Address dst_address, ::ams::svc::Handle process_handle, uint64_t src_address, ::ams::svc::Size size) {
                    return ::svcMapProcessMemory(reinterpret_cast<void *>(static_cast<uintptr_t>(dst_address)), process_handle, src_address, size);
                }

                ALWAYS_INLINE Result UnmapProcessMemory(::ams::svc::Address dst_address, ::ams::svc::Handle process_handle, uint64_t src_address, ::ams::svc::Size size) {
                    return ::svcUnmapProcessMemory(reinterpret_cast<void *>(static_cast<uintptr_t>(dst_address)), process_handle, src_address, size);
                }

                ALWAYS_INLINE Result QueryProcessMemory(::ams::svc::UserPointer< ::ams::svc::lp64::MemoryInfo *> out_memory_info, ::ams::svc::PageInfo *out_page_info, ::ams::svc::Handle process_handle, uint64_t address) {
                    return ::svcQueryProcessMemory(reinterpret_cast<::MemoryInfo *>(out_memory_info.GetPointerUnsafe()), reinterpret_cast<u32 *>(out_page_info), process_handle, address);
                }

                ALWAYS_INLINE Result MapProcessCodeMemory(::ams::svc::Handle process_handle, uint64_t dst_address, uint64_t src_address, uint64_t size) {
                    return ::svcMapProcessCodeMemory(process_handle, dst_address, src_address, size);
                }

                ALWAYS_INLINE Result UnmapProcessCodeMemory(::ams::svc::Handle process_handle, uint64_t dst_address, uint64_t src_address, uint64_t size) {
                    return ::svcUnmapProcessCodeMemory(process_handle, dst_address, src_address, size);
                }

                ALWAYS_INLINE Result CreateProcess(::ams::svc::Handle *out_handle, ::ams::svc::UserPointer<const ::ams::svc::lp64::CreateProcessParameter *> parameters, ::ams::svc::UserPointer<const uint32_t *> caps, int32_t num_caps) {
                    return ::svcCreateProcess(out_handle, parameters.GetPointerUnsafe(), caps.GetPointerUnsafe(), num_caps);
                }

                ALWAYS_INLINE Result StartProcess(::ams::svc::Handle process_handle, int32_t priority, int32_t core_id, uint64_t main_thread_stack_size) {
                    return ::svcStartProcess(process_handle, priority, core_id, main_thread_stack_size);
                }

                ALWAYS_INLINE Result TerminateProcess(::ams::svc::Handle process_handle) {
                    return ::svcTerminateProcess(process_handle);
                }

                ALWAYS_INLINE Result GetProcessInfo(int64_t *out_info, ::ams::svc::Handle process_handle, ::ams::svc::ProcessInfoType info_type) {
                    return ::svcGetProcessInfo(out_info, process_handle, static_cast<::ProcessInfoType>(info_type));
                }

                ALWAYS_INLINE Result CreateResourceLimit(::ams::svc::Handle *out_handle) {
                    return ::svcCreateResourceLimit(out_handle);
                }

                ALWAYS_INLINE Result SetResourceLimitLimitValue(::ams::svc::Handle resource_limit_handle, ::ams::svc::LimitableResource which, int64_t limit_value) {
                    return ::svcSetResourceLimitLimitValue(resource_limit_handle, static_cast<::LimitableResource>(which), limit_value);
                }

                ALWAYS_INLINE void CallSecureMonitor(::ams::svc::lp64::SecureMonitorArguments *args) {
                    ::svcCallSecureMonitor(reinterpret_cast<::SecmonArgs *>(args));
                }

            }

        #endif

        ALWAYS_INLINE bool IsKernelMesosphere() {
            uint64_t dummy;
            return R_SUCCEEDED(::ams::svc::GetInfo(std::addressof(dummy), ::ams::svc::InfoType_MesosphereMeta, ::ams::svc::InvalidHandle, ::ams::svc::MesosphereMetaInfo_KernelVersion));
        }

    }

#endif