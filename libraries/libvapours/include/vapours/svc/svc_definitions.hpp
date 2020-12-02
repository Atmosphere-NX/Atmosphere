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

#pragma once
#include <vapours/svc/svc_common.hpp>
#include <vapours/svc/svc_types.hpp>

#define AMS_SVC_KERN_INPUT_HANDLER(TYPE, NAME)  TYPE NAME
#define AMS_SVC_KERN_OUTPUT_HANDLER(TYPE, NAME) TYPE *NAME
#define AMS_SVC_KERN_INPTR_HANDLER(TYPE, NAME)  ::ams::kern::svc::KUserPointer<const TYPE *> NAME
#define AMS_SVC_KERN_OUTPTR_HANDLER(TYPE, NAME) ::ams::kern::svc::KUserPointer<TYPE *> NAME

#define AMS_SVC_USER_INPUT_HANDLER(TYPE, NAME)  TYPE NAME
#define AMS_SVC_USER_OUTPUT_HANDLER(TYPE, NAME) TYPE *NAME
#define AMS_SVC_USER_INPTR_HANDLER(TYPE, NAME)  ::ams::svc::UserPointer<const TYPE *> NAME
#define AMS_SVC_USER_OUTPTR_HANDLER(TYPE, NAME) ::ams::svc::UserPointer<TYPE *> NAME

#define AMS_SVC_FOREACH_DEFINITION_IMPL(HANDLER, NAMESPACE, INPUT, OUTPUT, INPTR, OUTPTR)                                                                                                                                                                                                                                                   \
    HANDLER(0x01, Result,  SetHeapSize,                    OUTPUT(::ams::svc::Address, out_address), INPUT(::ams::svc::Size, size))                                                                                                                                                                                                         \
    HANDLER(0x02, Result,  SetMemoryPermission,            INPUT(::ams::svc::Address, address), INPUT(::ams::svc::Size, size), INPUT(::ams::svc::MemoryPermission, perm))                                                                                                                                                                   \
    HANDLER(0x03, Result,  SetMemoryAttribute,             INPUT(::ams::svc::Address, address), INPUT(::ams::svc::Size, size), INPUT(uint32_t, mask), INPUT(uint32_t, attr))                                                                                                                                                                \
    HANDLER(0x04, Result,  MapMemory,                      INPUT(::ams::svc::Address, dst_address), INPUT(::ams::svc::Address, src_address), INPUT(::ams::svc::Size, size))                                                                                                                                                                 \
    HANDLER(0x05, Result,  UnmapMemory,                    INPUT(::ams::svc::Address, dst_address), INPUT(::ams::svc::Address, src_address), INPUT(::ams::svc::Size, size))                                                                                                                                                                 \
    HANDLER(0x06, Result,  QueryMemory,                    OUTPTR(::ams::svc::NAMESPACE::MemoryInfo, out_memory_info), OUTPUT(::ams::svc::PageInfo, out_page_info), INPUT(::ams::svc::Address, address))                                                                                                                                    \
    HANDLER(0x07, void,    ExitProcess)                                                                                                                                                                                                                                                                                                     \
    HANDLER(0x08, Result,  CreateThread,                   OUTPUT(::ams::svc::Handle, out_handle), INPUT(::ams::svc::ThreadFunc, func), INPUT(::ams::svc::Address, arg), INPUT(::ams::svc::Address, stack_bottom), INPUT(int32_t, priority), INPUT(int32_t, core_id))                                                                       \
    HANDLER(0x09, Result,  StartThread,                    INPUT(::ams::svc::Handle, thread_handle))                                                                                                                                                                                                                                        \
    HANDLER(0x0A, void,    ExitThread)                                                                                                                                                                                                                                                                                                      \
    HANDLER(0x0B, void,    SleepThread,                    INPUT(int64_t, ns))                                                                                                                                                                                                                                                              \
    HANDLER(0x0C, Result,  GetThreadPriority,              OUTPUT(int32_t, out_priority), INPUT(::ams::svc::Handle, thread_handle))                                                                                                                                                                                                         \
    HANDLER(0x0D, Result,  SetThreadPriority,              INPUT(::ams::svc::Handle, thread_handle), INPUT(int32_t, priority))                                                                                                                                                                                                              \
    HANDLER(0x0E, Result,  GetThreadCoreMask,              OUTPUT(int32_t, out_core_id), OUTPUT(uint64_t, out_affinity_mask), INPUT(::ams::svc::Handle, thread_handle))                                                                                                                                                                     \
    HANDLER(0x0F, Result,  SetThreadCoreMask,              INPUT(::ams::svc::Handle, thread_handle), INPUT(int32_t, core_id), INPUT(uint64_t, affinity_mask))                                                                                                                                                                               \
    HANDLER(0x10, int32_t, GetCurrentProcessorNumber)                                                                                                                                                                                                                                                                                       \
    HANDLER(0x11, Result,  SignalEvent,                    INPUT(::ams::svc::Handle, event_handle))                                                                                                                                                                                                                                         \
    HANDLER(0x12, Result,  ClearEvent,                     INPUT(::ams::svc::Handle, event_handle))                                                                                                                                                                                                                                         \
    HANDLER(0x13, Result,  MapSharedMemory,                INPUT(::ams::svc::Handle, shmem_handle), INPUT(::ams::svc::Address, address), INPUT(::ams::svc::Size, size), INPUT(::ams::svc::MemoryPermission, map_perm))                                                                                                                      \
    HANDLER(0x14, Result,  UnmapSharedMemory,              INPUT(::ams::svc::Handle, shmem_handle), INPUT(::ams::svc::Address, address), INPUT(::ams::svc::Size, size))                                                                                                                                                                     \
    HANDLER(0x15, Result,  CreateTransferMemory,           OUTPUT(::ams::svc::Handle, out_handle), INPUT(::ams::svc::Address, address), INPUT(::ams::svc::Size, size), INPUT(::ams::svc::MemoryPermission, map_perm))                                                                                                                       \
    HANDLER(0x16, Result,  CloseHandle,                    INPUT(::ams::svc::Handle, handle))                                                                                                                                                                                                                                               \
    HANDLER(0x17, Result,  ResetSignal,                    INPUT(::ams::svc::Handle, handle))                                                                                                                                                                                                                                               \
    HANDLER(0x18, Result,  WaitSynchronization,            OUTPUT(int32_t, out_index), INPTR(::ams::svc::Handle, handles), INPUT(int32_t, num_handles), INPUT(int64_t, timeout_ns))                                                                                                                                                         \
    HANDLER(0x19, Result,  CancelSynchronization,          INPUT(::ams::svc::Handle, handle))                                                                                                                                                                                                                                               \
    HANDLER(0x1A, Result,  ArbitrateLock,                  INPUT(::ams::svc::Handle, thread_handle), INPUT(::ams::svc::Address, address), INPUT(uint32_t, tag))                                                                                                                                                                             \
    HANDLER(0x1B, Result,  ArbitrateUnlock,                INPUT(::ams::svc::Address, address))                                                                                                                                                                                                                                             \
    HANDLER(0x1C, Result,  WaitProcessWideKeyAtomic,       INPUT(::ams::svc::Address, address), INPUT(::ams::svc::Address, cv_key), INPUT(uint32_t, tag), INPUT(int64_t, timeout_ns))                                                                                                                                                       \
    HANDLER(0x1D, void,    SignalProcessWideKey,           INPUT(::ams::svc::Address, cv_key), INPUT(int32_t, count))                                                                                                                                                                                                                       \
    HANDLER(0x1E, int64_t, GetSystemTick)                                                                                                                                                                                                                                                                                                   \
    HANDLER(0x1F, Result,  ConnectToNamedPort,             OUTPUT(::ams::svc::Handle, out_handle), INPTR(char, name))                                                                                                                                                                                                                       \
    HANDLER(0x20, Result,  SendSyncRequestLight,           INPUT(::ams::svc::Handle, session_handle))                                                                                                                                                                                                                                       \
    HANDLER(0x21, Result,  SendSyncRequest,                INPUT(::ams::svc::Handle, session_handle))                                                                                                                                                                                                                                       \
    HANDLER(0x22, Result,  SendSyncRequestWithUserBuffer,  INPUT(::ams::svc::Address, message_buffer), INPUT(::ams::svc::Size, message_buffer_size), INPUT(::ams::svc::Handle, session_handle))                                                                                                                                             \
    HANDLER(0x23, Result,  SendAsyncRequestWithUserBuffer, OUTPUT(::ams::svc::Handle, out_event_handle), INPUT(::ams::svc::Address, message_buffer), INPUT(::ams::svc::Size, message_buffer_size), INPUT(::ams::svc::Handle, session_handle))                                                                                               \
    HANDLER(0x24, Result,  GetProcessId,                   OUTPUT(uint64_t, out_process_id), INPUT(::ams::svc::Handle, process_handle))                                                                                                                                                                                                     \
    HANDLER(0x25, Result,  GetThreadId,                    OUTPUT(uint64_t, out_thread_id), INPUT(::ams::svc::Handle, thread_handle))                                                                                                                                                                                                       \
    HANDLER(0x26, void,    Break,                          INPUT(::ams::svc::BreakReason, break_reason), INPUT(::ams::svc::Address, arg), INPUT(::ams::svc::Size, size))                                                                                                                                                                    \
    HANDLER(0x27, Result,  OutputDebugString,              INPTR(char, debug_str), INPUT(::ams::svc::Size, len))                                                                                                                                                                                                                            \
    HANDLER(0x28, void,    ReturnFromException,            INPUT(::ams::Result, result))                                                                                                                                                                                                                                                    \
    HANDLER(0x29, Result,  GetInfo,                        OUTPUT(uint64_t, out), INPUT(::ams::svc::InfoType, info_type), INPUT(::ams::svc::Handle, handle), INPUT(uint64_t, info_subtype))                                                                                                                                                 \
    HANDLER(0x2A, void,    FlushEntireDataCache)                                                                                                                                                                                                                                                                                            \
    HANDLER(0x2B, Result,  FlushDataCache,                 INPUT(::ams::svc::Address, address), INPUT(::ams::svc::Size, size))                                                                                                                                                                                                              \
    HANDLER(0x2C, Result,  MapPhysicalMemory,              INPUT(::ams::svc::Address, address), INPUT(::ams::svc::Size, size))                                                                                                                                                                                                              \
    HANDLER(0x2D, Result,  UnmapPhysicalMemory,            INPUT(::ams::svc::Address, address), INPUT(::ams::svc::Size, size))                                                                                                                                                                                                              \
    HANDLER(0x2E, Result,  GetDebugFutureThreadInfo,       OUTPUT(::ams::svc::NAMESPACE::LastThreadContext, out_context), OUTPUT(uint64_t, thread_id), INPUT(::ams::svc::Handle, debug_handle), INPUT(int64_t, ns))                                                                                                                         \
    HANDLER(0x2F, Result,  GetLastThreadInfo,              OUTPUT(::ams::svc::NAMESPACE::LastThreadContext, out_context), OUTPUT(::ams::svc::Address, out_tls_address), OUTPUT(uint32_t, out_flags))                                                                                                                                        \
    HANDLER(0x30, Result,  GetResourceLimitLimitValue,     OUTPUT(int64_t, out_limit_value), INPUT(::ams::svc::Handle, resource_limit_handle), INPUT(::ams::svc::LimitableResource, which))                                                                                                                                                 \
    HANDLER(0x31, Result,  GetResourceLimitCurrentValue,   OUTPUT(int64_t, out_current_value), INPUT(::ams::svc::Handle, resource_limit_handle), INPUT(::ams::svc::LimitableResource, which))                                                                                                                                               \
    HANDLER(0x32, Result,  SetThreadActivity,              INPUT(::ams::svc::Handle, thread_handle), INPUT(::ams::svc::ThreadActivity, thread_activity))                                                                                                                                                                                    \
    HANDLER(0x33, Result,  GetThreadContext3,              OUTPTR(::ams::svc::ThreadContext, out_context), INPUT(::ams::svc::Handle, thread_handle))                                                                                                                                                                                        \
    HANDLER(0x34, Result,  WaitForAddress,                 INPUT(::ams::svc::Address, address), INPUT(::ams::svc::ArbitrationType, arb_type), INPUT(int32_t, value), INPUT(int64_t, timeout_ns))                                                                                                                                            \
    HANDLER(0x35, Result,  SignalToAddress,                INPUT(::ams::svc::Address, address), INPUT(::ams::svc::SignalType, signal_type), INPUT(int32_t, value), INPUT(int32_t, count))                                                                                                                                                   \
    HANDLER(0x36, void,    SynchronizePreemptionState)                                                                                                                                                                                                                                                                                      \
    HANDLER(0x37, Result,  GetResourceLimitPeakValue,      OUTPUT(int64_t, out_peak_value), INPUT(::ams::svc::Handle, resource_limit_handle), INPUT(::ams::svc::LimitableResource, which))                                                                                                                                                  \
                                                                                                                                                                                                                                                                                                                                            \
    HANDLER(0x39, Result,  Unknown39)                                                                                                                                                                                                                                                                                                       \
    HANDLER(0x3A, Result,  Unknown3A)                                                                                                                                                                                                                                                                                                       \
                                                                                                                                                                                                                                                                                                                                            \
    HANDLER(0x3C, void,    KernelDebug,                    INPUT(::ams::svc::KernelDebugType, kern_debug_type), INPUT(uint64_t, arg0), INPUT(uint64_t, arg1), INPUT(uint64_t, arg2))                                                                                                                                                        \
    HANDLER(0x3D, void,    ChangeKernelTraceState,         INPUT(::ams::svc::KernelTraceState, kern_trace_state))                                                                                                                                                                                                                           \
                                                                                                                                                                                                                                                                                                                                            \
    HANDLER(0x40, Result,  CreateSession,                  OUTPUT(::ams::svc::Handle, out_server_session_handle), OUTPUT(::ams::svc::Handle, out_client_session_handle), INPUT(bool, is_light), INPUT(::ams::svc::Address, name))                                                                                                           \
    HANDLER(0x41, Result,  AcceptSession,                  OUTPUT(::ams::svc::Handle, out_handle), INPUT(::ams::svc::Handle, port))                                                                                                                                                                                                         \
    HANDLER(0x42, Result,  ReplyAndReceiveLight,           INPUT(::ams::svc::Handle, handle))                                                                                                                                                                                                                                               \
    HANDLER(0x43, Result,  ReplyAndReceive,                OUTPUT(int32_t, out_index), INPTR(::ams::svc::Handle, handles), INPUT(int32_t, num_handles), INPUT(::ams::svc::Handle, reply_target), INPUT(int64_t, timeout_ns))                                                                                                                \
    HANDLER(0x44, Result,  ReplyAndReceiveWithUserBuffer,  OUTPUT(int32_t, out_index), INPUT(::ams::svc::Address, message_buffer), INPUT(::ams::svc::Size, message_buffer_size), INPTR(::ams::svc::Handle, handles), INPUT(int32_t, num_handles), INPUT(::ams::svc::Handle, reply_target), INPUT(int64_t, timeout_ns))                      \
    HANDLER(0x45, Result,  CreateEvent,                    OUTPUT(::ams::svc::Handle, out_write_handle), OUTPUT(::ams::svc::Handle, out_read_handle))                                                                                                                                                                                       \
    HANDLER(0x46, Result,  Unknown46)                                                                                                                                                                                                                                                                                                       \
    HANDLER(0x47, Result,  Unknown47)                                                                                                                                                                                                                                                                                                       \
    HANDLER(0x48, Result,  MapPhysicalMemoryUnsafe,        INPUT(::ams::svc::Address, address), INPUT(::ams::svc::Size, size))                                                                                                                                                                                                              \
    HANDLER(0x49, Result,  UnmapPhysicalMemoryUnsafe,      INPUT(::ams::svc::Address, address), INPUT(::ams::svc::Size, size))                                                                                                                                                                                                              \
    HANDLER(0x4A, Result,  SetUnsafeLimit,                 INPUT(::ams::svc::Size, limit))                                                                                                                                                                                                                                                  \
    HANDLER(0x4B, Result,  CreateCodeMemory,               OUTPUT(::ams::svc::Handle, out_handle), INPUT(::ams::svc::Address, address), INPUT(::ams::svc::Size, size))                                                                                                                                                                      \
    HANDLER(0x4C, Result,  ControlCodeMemory,              INPUT(::ams::svc::Handle, code_memory_handle), INPUT(::ams::svc::CodeMemoryOperation, operation), INPUT(uint64_t, address), INPUT(uint64_t, size), INPUT(::ams::svc::MemoryPermission, perm))                                                                                    \
    HANDLER(0x4D, void,    SleepSystem)                                                                                                                                                                                                                                                                                                     \
    HANDLER(0x4E, Result,  ReadWriteRegister,              OUTPUT(uint32_t, out_value), INPUT(::ams::svc::PhysicalAddress, address), INPUT(uint32_t, mask), INPUT(uint32_t, value))                                                                                                                                                         \
    HANDLER(0x4F, Result,  SetProcessActivity,             INPUT(::ams::svc::Handle, process_handle), INPUT(::ams::svc::ProcessActivity, process_activity))                                                                                                                                                                                 \
    HANDLER(0x50, Result,  CreateSharedMemory,             OUTPUT(::ams::svc::Handle, out_handle), INPUT(::ams::svc::Size, size), INPUT(::ams::svc::MemoryPermission, owner_perm), INPUT(::ams::svc::MemoryPermission, remote_perm))                                                                                                        \
    HANDLER(0x51, Result,  MapTransferMemory,              INPUT(::ams::svc::Handle, trmem_handle), INPUT(::ams::svc::Address, address), INPUT(::ams::svc::Size, size), INPUT(::ams::svc::MemoryPermission, owner_perm))                                                                                                                    \
    HANDLER(0x52, Result,  UnmapTransferMemory,            INPUT(::ams::svc::Handle, trmem_handle), INPUT(::ams::svc::Address, address), INPUT(::ams::svc::Size, size))                                                                                                                                                                     \
    HANDLER(0x53, Result,  CreateInterruptEvent,           OUTPUT(::ams::svc::Handle, out_read_handle), INPUT(int32_t, interrupt_id), INPUT(::ams::svc::InterruptType, interrupt_type))                                                                                                                                                     \
    HANDLER(0x54, Result,  QueryPhysicalAddress,           OUTPUT(::ams::svc::NAMESPACE::PhysicalMemoryInfo, out_info), INPUT(::ams::svc::Address, address))                                                                                                                                                                                \
    HANDLER(0x55, Result,  QueryIoMapping,                 OUTPUT(::ams::svc::Address, out_address), OUTPUT(::ams::svc::Size, out_size), INPUT(::ams::svc::PhysicalAddress, physical_address), INPUT(::ams::svc::Size, size))                                                                                                               \
    HANDLER(0x56, Result,  CreateDeviceAddressSpace,       OUTPUT(::ams::svc::Handle, out_handle), INPUT(uint64_t, das_address), INPUT(uint64_t, das_size))                                                                                                                                                                                 \
    HANDLER(0x57, Result,  AttachDeviceAddressSpace,       INPUT(::ams::svc::DeviceName, device_name), INPUT(::ams::svc::Handle, das_handle))                                                                                                                                                                                               \
    HANDLER(0x58, Result,  DetachDeviceAddressSpace,       INPUT(::ams::svc::DeviceName, device_name), INPUT(::ams::svc::Handle, das_handle))                                                                                                                                                                                               \
    HANDLER(0x59, Result,  MapDeviceAddressSpaceByForce,   INPUT(::ams::svc::Handle, das_handle), INPUT(::ams::svc::Handle, process_handle), INPUT(uint64_t, process_address), INPUT(::ams::svc::Size, size), INPUT(uint64_t, device_address), INPUT(::ams::svc::MemoryPermission, device_perm))                                            \
    HANDLER(0x5A, Result,  MapDeviceAddressSpaceAligned,   INPUT(::ams::svc::Handle, das_handle), INPUT(::ams::svc::Handle, process_handle), INPUT(uint64_t, process_address), INPUT(::ams::svc::Size, size), INPUT(uint64_t, device_address), INPUT(::ams::svc::MemoryPermission, device_perm))                                            \
    HANDLER(0x5B, Result,  MapDeviceAddressSpace,          OUTPUT(::ams::svc::Size, out_mapped_size), INPUT(::ams::svc::Handle, das_handle), INPUT(::ams::svc::Handle, process_handle), INPUT(uint64_t, process_address), INPUT(::ams::svc::Size, size), INPUT(uint64_t, device_address), INPUT(::ams::svc::MemoryPermission, device_perm)) \
    HANDLER(0x5C, Result,  UnmapDeviceAddressSpace,        INPUT(::ams::svc::Handle, das_handle), INPUT(::ams::svc::Handle, process_handle), INPUT(uint64_t, process_address), INPUT(::ams::svc::Size, size), INPUT(uint64_t, device_address))                                                                                              \
    HANDLER(0x5D, Result,  InvalidateProcessDataCache,     INPUT(::ams::svc::Handle, process_handle), INPUT(uint64_t, address), INPUT(uint64_t, size))                                                                                                                                                                                      \
    HANDLER(0x5E, Result,  StoreProcessDataCache,          INPUT(::ams::svc::Handle, process_handle), INPUT(uint64_t, address), INPUT(uint64_t, size))                                                                                                                                                                                      \
    HANDLER(0x5F, Result,  FlushProcessDataCache,          INPUT(::ams::svc::Handle, process_handle), INPUT(uint64_t, address), INPUT(uint64_t, size))                                                                                                                                                                                      \
    HANDLER(0x60, Result,  DebugActiveProcess,             OUTPUT(::ams::svc::Handle, out_handle), INPUT(uint64_t, process_id))                                                                                                                                                                                                             \
    HANDLER(0x61, Result,  BreakDebugProcess,              INPUT(::ams::svc::Handle, debug_handle))                                                                                                                                                                                                                                         \
    HANDLER(0x62, Result,  TerminateDebugProcess,          INPUT(::ams::svc::Handle, debug_handle))                                                                                                                                                                                                                                         \
    HANDLER(0x63, Result,  GetDebugEvent,                  OUTPTR(::ams::svc::NAMESPACE::DebugEventInfo, out_info), INPUT(::ams::svc::Handle, debug_handle))                                                                                                                                                                                \
    HANDLER(0x64, Result,  ContinueDebugEvent,             INPUT(::ams::svc::Handle, debug_handle), INPUT(uint32_t, flags), INPTR(uint64_t, thread_ids), INPUT(int32_t, num_thread_ids))                                                                                                                                                    \
    HANDLER(0x65, Result,  GetProcessList,                 OUTPUT(int32_t, out_num_processes), OUTPTR(uint64_t, out_process_ids), INPUT(int32_t, max_out_count))                                                                                                                                                                            \
    HANDLER(0x66, Result,  GetThreadList,                  OUTPUT(int32_t, out_num_threads), OUTPTR(uint64_t, out_thread_ids), INPUT(int32_t, max_out_count), INPUT(::ams::svc::Handle, debug_handle))                                                                                                                                      \
    HANDLER(0x67, Result,  GetDebugThreadContext,          OUTPTR(::ams::svc::ThreadContext, out_context), INPUT(::ams::svc::Handle, debug_handle), INPUT(uint64_t, thread_id), INPUT(uint32_t, context_flags))                                                                                                                             \
    HANDLER(0x68, Result,  SetDebugThreadContext,          INPUT(::ams::svc::Handle, debug_handle), INPUT(uint64_t, thread_id), INPTR(::ams::svc::ThreadContext, context), INPUT(uint32_t, context_flags))                                                                                                                                  \
    HANDLER(0x69, Result,  QueryDebugProcessMemory,        OUTPTR(::ams::svc::NAMESPACE::MemoryInfo, out_memory_info), OUTPUT(::ams::svc::PageInfo, out_page_info), INPUT(::ams::svc::Handle, process_handle), INPUT(::ams::svc::Address, address))                                                                                         \
    HANDLER(0x6A, Result,  ReadDebugProcessMemory,         INPUT(::ams::svc::Address, buffer), INPUT(::ams::svc::Handle, debug_handle), INPUT(::ams::svc::Address, address), INPUT(::ams::svc::Size, size))                                                                                                                                 \
    HANDLER(0x6B, Result,  WriteDebugProcessMemory,        INPUT(::ams::svc::Handle, debug_handle), INPUT(::ams::svc::Address, buffer), INPUT(::ams::svc::Address, address), INPUT(::ams::svc::Size, size))                                                                                                                                 \
    HANDLER(0x6C, Result,  SetHardwareBreakPoint,          INPUT(::ams::svc::HardwareBreakPointRegisterName, name), INPUT(uint64_t, flags), INPUT(uint64_t, value))                                                                                                                                                                         \
    HANDLER(0x6D, Result,  GetDebugThreadParam,            OUTPUT(uint64_t, out_64), OUTPUT(uint32_t, out_32), INPUT(::ams::svc::Handle, debug_handle), INPUT(uint64_t, thread_id), INPUT(::ams::svc::DebugThreadParam, param))                                                                                                             \
                                                                                                                                                                                                                                                                                                                                            \
    HANDLER(0x6F, Result,  GetSystemInfo,                  OUTPUT(uint64_t, out), INPUT(::ams::svc::SystemInfoType, info_type), INPUT(::ams::svc::Handle, handle), INPUT(uint64_t, info_subtype))                                                                                                                                           \
    HANDLER(0x70, Result,  CreatePort,                     OUTPUT(::ams::svc::Handle, out_server_handle), OUTPUT(::ams::svc::Handle, out_client_handle), INPUT(int32_t, max_sessions), INPUT(bool, is_light), INPUT(::ams::svc::Address, name))                                                                                             \
    HANDLER(0x71, Result,  ManageNamedPort,                OUTPUT(::ams::svc::Handle, out_server_handle), INPTR(char, name), INPUT(int32_t, max_sessions))                                                                                                                                                                                  \
    HANDLER(0x72, Result,  ConnectToPort,                  OUTPUT(::ams::svc::Handle, out_handle), INPUT(::ams::svc::Handle, port))                                                                                                                                                                                                         \
    HANDLER(0x73, Result,  SetProcessMemoryPermission,     INPUT(::ams::svc::Handle, process_handle), INPUT(uint64_t, address), INPUT(uint64_t, size), INPUT(::ams::svc::MemoryPermission, perm))                                                                                                                                           \
    HANDLER(0x74, Result,  MapProcessMemory,               INPUT(::ams::svc::Address, dst_address), INPUT(::ams::svc::Handle, process_handle), INPUT(uint64_t, src_address), INPUT(::ams::svc::Size, size))                                                                                                                                 \
    HANDLER(0x75, Result,  UnmapProcessMemory,             INPUT(::ams::svc::Address, dst_address), INPUT(::ams::svc::Handle, process_handle), INPUT(uint64_t, src_address), INPUT(::ams::svc::Size, size))                                                                                                                                 \
    HANDLER(0x76, Result,  QueryProcessMemory,             OUTPTR(::ams::svc::NAMESPACE::MemoryInfo, out_memory_info), OUTPUT(::ams::svc::PageInfo, out_page_info), INPUT(::ams::svc::Handle, process_handle), INPUT(uint64_t, address))                                                                                                    \
    HANDLER(0x77, Result,  MapProcessCodeMemory,           INPUT(::ams::svc::Handle, process_handle), INPUT(uint64_t, dst_address), INPUT(uint64_t, src_address), INPUT(uint64_t, size))                                                                                                                                                    \
    HANDLER(0x78, Result,  UnmapProcessCodeMemory,         INPUT(::ams::svc::Handle, process_handle), INPUT(uint64_t, dst_address), INPUT(uint64_t, src_address), INPUT(uint64_t, size))                                                                                                                                                    \
    HANDLER(0x79, Result,  CreateProcess,                  OUTPUT(::ams::svc::Handle, out_handle), INPTR(::ams::svc::NAMESPACE::CreateProcessParameter, parameters), INPTR(uint32_t, caps), INPUT(int32_t, num_caps))                                                                                                                       \
    HANDLER(0x7A, Result,  StartProcess,                   INPUT(::ams::svc::Handle, process_handle), INPUT(int32_t, priority), INPUT(int32_t, core_id), INPUT(uint64_t, main_thread_stack_size))                                                                                                                                           \
    HANDLER(0x7B, Result,  TerminateProcess,               INPUT(::ams::svc::Handle, process_handle))                                                                                                                                                                                                                                       \
    HANDLER(0x7C, Result,  GetProcessInfo,                 OUTPUT(int64_t, out_info), INPUT(::ams::svc::Handle, process_handle), INPUT(::ams::svc::ProcessInfoType, info_type))                                                                                                                                                             \
    HANDLER(0x7D, Result,  CreateResourceLimit,            OUTPUT(::ams::svc::Handle, out_handle))                                                                                                                                                                                                                                          \
    HANDLER(0x7E, Result,  SetResourceLimitLimitValue,     INPUT(::ams::svc::Handle, resource_limit_handle), INPUT(::ams::svc::LimitableResource, which), INPUT(int64_t, limit_value))                                                                                                                                                      \
    HANDLER(0x7F, void,    CallSecureMonitor,              OUTPUT(::ams::svc::NAMESPACE::SecureMonitorArguments, args))                                                                                                                                                                                                                     \
                                                                                                                                                                                                                                                                                                                                            \
    HANDLER(0x2E, Result,  LegacyGetFutureThreadInfo,      OUTPUT(::ams::svc::NAMESPACE::LastThreadContext, out_context), OUTPUT(::ams::svc::Address, out_tls_address), OUTPUT(uint32_t, out_flags), INPUT(int64_t, ns))                                                                                                                    \
    HANDLER(0x55, Result,  LegacyQueryIoMapping,           OUTPUT(::ams::svc::Address, out_address), INPUT(::ams::svc::PhysicalAddress, physical_address), INPUT(::ams::svc::Size, size))                                                                                                                                                   \
    HANDLER(0x64, Result,  LegacyContinueDebugEvent,       INPUT(::ams::svc::Handle, debug_handle), INPUT(uint32_t, flags), INPUT(uint64_t, thread_id))

#define AMS_SVC_FOREACH_USER_DEFINITION(HANDLER, NAMESPACE) AMS_SVC_FOREACH_DEFINITION_IMPL(HANDLER, NAMESPACE, AMS_SVC_USER_INPUT_HANDLER, AMS_SVC_USER_OUTPUT_HANDLER, AMS_SVC_USER_INPTR_HANDLER, AMS_SVC_USER_OUTPTR_HANDLER)
#define AMS_SVC_FOREACH_KERN_DEFINITION(HANDLER, NAMESPACE) AMS_SVC_FOREACH_DEFINITION_IMPL(HANDLER, NAMESPACE, AMS_SVC_KERN_INPUT_HANDLER, AMS_SVC_KERN_OUTPUT_HANDLER, AMS_SVC_KERN_INPTR_HANDLER, AMS_SVC_KERN_OUTPTR_HANDLER)

#define AMS_SVC_DECLARE_FUNCTION_PROTOTYPE(ID, RETURN_TYPE, NAME, ...) \
    RETURN_TYPE NAME(__VA_ARGS__);

namespace ams::svc {

    #define AMS_SVC_DEFINE_ID_ENUM_MEMBER(ID, RETURN_TYPE, NAME, ...) \
    SvcId_##NAME = ID,

    enum SvcId : u32 {
        AMS_SVC_FOREACH_KERN_DEFINITION(AMS_SVC_DEFINE_ID_ENUM_MEMBER, _)
    };

    #undef AMS_SVC_DEFINE_ID_ENUM_MEMBER

}

#ifdef ATMOSPHERE_IS_STRATOSPHERE

namespace ams::svc {

    namespace aarch64::lp64 {

        AMS_SVC_FOREACH_USER_DEFINITION(AMS_SVC_DECLARE_FUNCTION_PROTOTYPE, lp64)

    }

    namespace aarch64::ilp32 {

        AMS_SVC_FOREACH_USER_DEFINITION(AMS_SVC_DECLARE_FUNCTION_PROTOTYPE, ilp32)

    }

    namespace aarch32 {

        AMS_SVC_FOREACH_USER_DEFINITION(AMS_SVC_DECLARE_FUNCTION_PROTOTYPE, ilp32)

    }

}

/* NOTE: Change this to 1 to test the SVC definitions for user-pointer validity. */
#if 0
namespace ams::svc::test {

    namespace impl {

        template<typename... Ts>
        struct Validator {
            private:
                std::array<bool, sizeof...(Ts)> valid;
            public:
                constexpr Validator(Ts... args) : valid{static_cast<bool>(args)...} { /* ... */ }

                constexpr bool IsValid() const {
                    for (size_t i = 0; i < sizeof...(Ts); i++) {
                        if (!this->valid[i]) {
                            return false;
                        }
                    }
                    return true;
                }
        };

    }


    #define AMS_SVC_TEST_EMPTY_HANDLER(TYPE, NAME)  true
    #define AMS_SVC_TEST_INPTR_HANDLER(TYPE, NAME)  (sizeof(::ams::svc::UserPointer<const TYPE *>) == sizeof(uintptr_t) && std::is_trivially_destructible<::ams::svc::UserPointer<const TYPE *>>::value)
    #define AMS_SVC_TEST_OUTPTR_HANDLER(TYPE, NAME) (sizeof(::ams::svc::UserPointer<TYPE *>) == sizeof(uintptr_t) && std::is_trivially_destructible<::ams::svc::UserPointer<TYPE *>>::value)

    #define AMS_SVC_TEST_VERIFY_USER_POINTERS(ID, RETURN_TYPE, NAME, ...) \
        static_assert(impl::Validator(__VA_ARGS__).IsValid(), "Invalid User Pointer in svc::" #NAME);

    AMS_SVC_FOREACH_DEFINITION_IMPL(AMS_SVC_TEST_VERIFY_USER_POINTERS, lp64,  AMS_SVC_TEST_EMPTY_HANDLER, AMS_SVC_TEST_EMPTY_HANDLER, AMS_SVC_TEST_INPTR_HANDLER, AMS_SVC_TEST_OUTPTR_HANDLER);
    AMS_SVC_FOREACH_DEFINITION_IMPL(AMS_SVC_TEST_VERIFY_USER_POINTERS, ilp32, AMS_SVC_TEST_EMPTY_HANDLER, AMS_SVC_TEST_EMPTY_HANDLER, AMS_SVC_TEST_INPTR_HANDLER, AMS_SVC_TEST_OUTPTR_HANDLER);

    #undef  AMS_SVC_TEST_VERIFY_USER_POINTERS
    #undef  AMS_SVC_TEST_INPTR_HANDLER
    #undef  AMS_SVC_TEST_OUTPTR_HANDLER
    #undef  AMS_SVC_TEST_EMPTY_HANDLER

}
#endif

#endif /* ATMOSPHERE_IS_STRATOSPHERE */

