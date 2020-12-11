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

namespace ams::kern::svc::impl {

    struct KUserPointerTag{};

}

namespace ams::svc {

    /* Utility classes required to encode information into the type system for SVC veneers. */
    class Size {
        private:
            size_t size;
        public:
            constexpr ALWAYS_INLINE Size(size_t s) : size(s) { /* ... */ }
            constexpr ALWAYS_INLINE operator size_t() { return this->size; }
    };
    static_assert(sizeof(Size) == sizeof(size_t));
    static_assert(std::is_trivially_destructible<Size>::value);

    class Address {
        private:
            uintptr_t uintptr;
        public:
            constexpr ALWAYS_INLINE Address(uintptr_t u) : uintptr(u) { /* ... */ }
            constexpr ALWAYS_INLINE operator uintptr_t() { return this->uintptr; }
    };
    static_assert(sizeof(Address) == sizeof(uintptr_t));
    static_assert(std::is_trivially_destructible<Address>::value);

    namespace impl {

        struct UserPointerTag{};

    }

    template<typename T>
    struct UserPointer : impl::UserPointerTag {
        public:
            static_assert(std::is_pointer<T>::value);
            static constexpr bool IsInput = std::is_const<typename std::remove_pointer<T>::type>::value;
        private:
            T pointer;
        public:
            constexpr ALWAYS_INLINE UserPointer(T p) : pointer(p) { /* ... */ }

            constexpr ALWAYS_INLINE T GetPointerUnsafe() { return this->pointer; }
    };

    template<typename T>
    static constexpr inline bool IsUserPointer = std::is_base_of<impl::UserPointerTag, T>::value;

    using ProgramId       = u64;
    using PhysicalAddress = u64;

    /* Memory types. */
    enum MemoryState : u32 {
        MemoryState_Free             = 0x00,
        MemoryState_Io               = 0x01,
        MemoryState_Static           = 0x02,
        MemoryState_Code             = 0x03,
        MemoryState_CodeData         = 0x04,
        MemoryState_Normal           = 0x05,
        MemoryState_Shared           = 0x06,
        MemoryState_Alias            = 0x07,
        MemoryState_AliasCode        = 0x08,
        MemoryState_AliasCodeData    = 0x09,
        MemoryState_Ipc              = 0x0A,
        MemoryState_Stack            = 0x0B,
        MemoryState_ThreadLocal      = 0x0C,
        MemoryState_Transfered       = 0x0D,
        MemoryState_SharedTransfered = 0x0E,
        MemoryState_SharedCode       = 0x0F,
        MemoryState_Inaccessible     = 0x10,
        MemoryState_NonSecureIpc     = 0x11,
        MemoryState_NonDeviceIpc     = 0x12,
        MemoryState_Kernel           = 0x13,
        MemoryState_GeneratedCode    = 0x14,
        MemoryState_CodeOut          = 0x15,
    };

    enum MemoryPermission : u32 {
        MemoryPermission_None        = (0 << 0),

        MemoryPermission_Read        = (1 << 0),
        MemoryPermission_Write       = (1 << 1),
        MemoryPermission_Execute     = (1 << 2),

        MemoryPermission_ReadWrite   = MemoryPermission_Read | MemoryPermission_Write,
        MemoryPermission_ReadExecute = MemoryPermission_Read | MemoryPermission_Execute,

        MemoryPermission_DontCare    = (1 << 28), /* For SharedMemory */
    };

    enum MemoryAttribute : u32 {
        MemoryAttribute_Locked       = (1 << 0),
        MemoryAttribute_IpcLocked    = (1 << 1),
        MemoryAttribute_DeviceShared = (1 << 2),
        MemoryAttribute_Uncached     = (1 << 3),
    };

    constexpr inline size_t HeapSizeAlignment = 2_MB;

    struct PageInfo {
        u32 flags;
    };

    enum MemoryRegionType {
        MemoryRegionType_None              = 0,
        MemoryRegionType_KernelTraceBuffer = 1,
        MemoryRegionType_OnMemoryBootImage = 2,
        MemoryRegionType_DTB               = 3,
        MemoryRegionType_Count,
    };

    /* Info Types. */
    enum InfoType : u32 {
        InfoType_CoreMask                       = 0,
        InfoType_PriorityMask                   = 1,
        InfoType_AliasRegionAddress             = 2,
        InfoType_AliasRegionSize                = 3,
        InfoType_HeapRegionAddress              = 4,
        InfoType_HeapRegionSize                 = 5,
        InfoType_TotalMemorySize                = 6,
        InfoType_UsedMemorySize                 = 7,
        InfoType_DebuggerAttached               = 8,
        InfoType_ResourceLimit                  = 9,
        InfoType_IdleTickCount                  = 10,
        InfoType_RandomEntropy                  = 11,
        InfoType_AslrRegionAddress              = 12,
        InfoType_AslrRegionSize                 = 13,
        InfoType_StackRegionAddress             = 14,
        InfoType_StackRegionSize                = 15,
        InfoType_SystemResourceSizeTotal        = 16,
        InfoType_SystemResourceSizeUsed         = 17,
        InfoType_ProgramId                      = 18,
        InfoType_InitialProcessIdRange          = 19,
        InfoType_UserExceptionContextAddress    = 20,
        InfoType_TotalNonSystemMemorySize       = 21,
        InfoType_UsedNonSystemMemorySize        = 22,
        InfoType_IsApplication                  = 23,
        InfoType_FreeThreadCount                = 24,

        InfoType_MesosphereMeta                 = 65000,

        InfoType_ThreadTickCount                = 0xF0000002,
    };

    enum TickCountInfo : u64 {
        TickCountInfo_Core0 = 0,
        TickCountInfo_Core1 = 1,
        TickCountInfo_Core2 = 2,
        TickCountInfo_Core3 = 3,

        TickCountInfo_Total = std::numeric_limits<s64>::max(),
    };

    enum MesosphereMetaInfo : u64 {
        MesosphereMetaInfo_KernelVersion   = 0,
        MesosphereMetaInfo_IsKTraceEnabled = 1,
    };

    enum SystemInfoType : u32 {
        SystemInfoType_TotalPhysicalMemorySize  = 0,
        SystemInfoType_UsedPhysicalMemorySize   = 1,
        SystemInfoType_InitialProcessIdRange    = 2,
    };

    enum InitialProcessIdRangeInfo : u64 {
        InitialProcessIdRangeInfo_Minimum = 0,
        InitialProcessIdRangeInfo_Maximum = 1,
    };

    enum PhysicalMemorySystemInfo : u64 {
        PhysicalMemorySystemInfo_Application  = 0,
        PhysicalMemorySystemInfo_Applet       = 1,
        PhysicalMemorySystemInfo_System       = 2,
        PhysicalMemorySystemInfo_SystemUnsafe = 3,
    };

    enum LastThreadInfoFlag : u32 {
        LastThreadInfoFlag_ThreadInSystemCall = (1u << 0),
    };

    enum LimitableResource : u32 {
        LimitableResource_PhysicalMemoryMax      = 0,
        LimitableResource_ThreadCountMax         = 1,
        LimitableResource_EventCountMax          = 2,
        LimitableResource_TransferMemoryCountMax = 3,
        LimitableResource_SessionCountMax        = 4,

        LimitableResource_Count,
    };

    enum CodeMemoryOperation : u32 {
        CodeMemoryOperation_Map            = 0,
        CodeMemoryOperation_MapToOwner     = 1,
        CodeMemoryOperation_Unmap          = 2,
        CodeMemoryOperation_UnmapFromOwner = 3,
    };

    /* Synchronization types. */
    enum SignalType : u32 {
        SignalType_Signal                               = 0,
        SignalType_SignalAndIncrementIfEqual            = 1,
        SignalType_SignalAndModifyByWaitingCountIfEqual = 2,
    };

    enum ArbitrationType : u32 {
        ArbitrationType_WaitIfLessThan             = 0,
        ArbitrationType_DecrementAndWaitIfLessThan = 1,
        ArbitrationType_WaitIfEqual                = 2,
    };

    enum YieldType : s64 {
        YieldType_WithoutCoreMigration = 0,
        YieldType_WithCoreMigration    = -1,
        YieldType_ToAnyThread          = -2,
    };

    enum InterruptType : u32 {
        InterruptType_Edge  = 0,
        InterruptType_Level = 1,
    };

    /* Thread types. */
    using ThreadFunc = ams::svc::Address;

#if defined(ATMOSPHERE_ARCH_ARM64)

    struct ThreadContext {
        u64  r[29];
        u64  fp;
        u64  lr;
        u64  sp;
        u64  pc;
        u32  pstate;
        u32  padding;
        u128 v[32];
        u32 fpcr;
        u32 fpsr;
        u64 tpidr;
    };
    static_assert(sizeof(ThreadContext) == 0x320);

#elif defined(ATMOSPHERE_ARCH_ARM)

    struct ThreadContext {
        u32 r[13];
        u32 sp;
        u32 lr;
        u32 pc;
        u32 cpsr;
        u32 padding;
        u64 fpu_registers[32];
        u32 fpscr;
        u32 fpexc;
        u32 tpidr;
    };

#else
    #error "Unknown Architecture for ams::svc::ThreadContext"
#endif

    enum ThreadSuspend : u32 {
        ThreadSuspend_Debug = (1 << 0),
        ThreadSuspend_User  = (1 << 1),
    };

    enum ThreadState : u32 {
        ThreadState_Waiting      = 0,
        ThreadState_Running      = 1,
        ThreadState_Terminated   = 4,
        ThreadState_Initializing = 5,
    };

    enum ThreadContextFlag : u32 {
        ThreadContextFlag_General    = (1 << 0),
        ThreadContextFlag_Control    = (1 << 1),
        ThreadContextFlag_Fpu        = (1 << 2),
        ThreadContextFlag_FpuControl = (1 << 3),

        ThreadContextFlag_All = (ThreadContextFlag_General | ThreadContextFlag_Control | ThreadContextFlag_Fpu | ThreadContextFlag_FpuControl),
    };

    enum ContinueFlag : u32 {
        ContinueFlag_ExceptionHandled     = (1u << 0),
        ContinueFlag_EnableExceptionEvent = (1u << 1),
        ContinueFlag_ContinueAll          = (1u << 2),
        ContinueFlag_ContinueOthers       = (1u << 3),

        ContinueFlag_AllMask              = (1u << 4) - 1,
    };

    enum ThreadExitReason : u32 {
        ThreadExitReason_ExitThread       = 0,
        ThreadExitReason_TerminateThread  = 1,
        ThreadExitReason_ExitProcess      = 2,
        ThreadExitReason_TerminateProcess = 3,
    };

    enum ThreadActivity : u32 {
        ThreadActivity_Runnable = 0,
        ThreadActivity_Paused   = 1,
    };

    constexpr inline s32 IdealCoreDontCare        = -1;
    constexpr inline s32 IdealCoreUseProcessValue = -2;
    constexpr inline s32 IdealCoreNoUpdate        = -3;

    constexpr inline s32 LowestThreadPriority  = 63;
    constexpr inline s32 HighestThreadPriority = 0;

    constexpr inline s32 SystemThreadPriorityHighest = 16;

    /* Process types. */
    enum ProcessInfoType : u32 {
        ProcessInfoType_ProcessState = 0,
    };

    enum ProcessState : u32 {
        ProcessState_Created         = 0,
        ProcessState_CreatedAttached = 1,
        ProcessState_Running         = 2,
        ProcessState_Crashed         = 3,
        ProcessState_RunningAttached = 4,
        ProcessState_Terminating     = 5,
        ProcessState_Terminated      = 6,
        ProcessState_DebugBreak      = 7,
    };

    enum ProcessExitReason : u32 {
        ProcessExitReason_ExitProcess      = 0,
        ProcessExitReason_TerminateProcess = 1,
        ProcessExitReason_Exception        = 2,
    };

    enum ProcessActivity : u32 {
        ProcessActivity_Runnable = 0,
        ProcessActivity_Paused   = 1,
    };

    enum CreateProcessFlag : u32 {
        /* Is 64 bit? */
        CreateProcessFlag_Is64Bit       = (1 << 0),

        /* What kind of address space? */
        CreateProcessFlag_AddressSpaceShift             = 1,
        CreateProcessFlag_AddressSpaceMask              = (7 << CreateProcessFlag_AddressSpaceShift),
        CreateProcessFlag_AddressSpace32Bit             = (0 << CreateProcessFlag_AddressSpaceShift),
        CreateProcessFlag_AddressSpace64BitDeprecated   = (1 << CreateProcessFlag_AddressSpaceShift),
        CreateProcessFlag_AddressSpace32BitWithoutAlias = (2 << CreateProcessFlag_AddressSpaceShift),
        CreateProcessFlag_AddressSpace64Bit             = (3 << CreateProcessFlag_AddressSpaceShift),

        /* Should JIT debug be done on crash? */
        CreateProcessFlag_EnableDebug   = (1 << 4),

        /* Should ASLR be enabled for the process? */
        CreateProcessFlag_EnableAslr    = (1 << 5),

        /* Is the process an application? */
        CreateProcessFlag_IsApplication = (1 << 6),

        /* 4.x deprecated: Should use secure memory? */
        CreateProcessFlag_DeprecatedUseSecureMemory = (1 << 7),

        /* 5.x+ Pool partition type. */
        CreateProcessFlag_PoolPartitionShift            = 7,
        CreateProcessFlag_PoolPartitionMask             = (0xF << CreateProcessFlag_PoolPartitionShift),
        CreateProcessFlag_PoolPartitionApplication      = (0 << CreateProcessFlag_PoolPartitionShift),
        CreateProcessFlag_PoolPartitionApplet           = (1 << CreateProcessFlag_PoolPartitionShift),
        CreateProcessFlag_PoolPartitionSystem           = (2 << CreateProcessFlag_PoolPartitionShift),
        CreateProcessFlag_PoolPartitionSystemNonSecure  = (3 << CreateProcessFlag_PoolPartitionShift),

        /* 7.x+ Should memory allocation be optimized? This requires IsApplication. */
        CreateProcessFlag_OptimizeMemoryAllocation = (1 << 11),

        /* 11.x+ DisableDeviceAddressSpaceMerge. */
        CreateProcessFlag_DisableDeviceAddressSpaceMerge = (1 << 12),

        /* Mask of all flags. */
        CreateProcessFlag_All = CreateProcessFlag_Is64Bit                  |
                                CreateProcessFlag_AddressSpaceMask         |
                                CreateProcessFlag_EnableDebug              |
                                CreateProcessFlag_EnableAslr               |
                                CreateProcessFlag_IsApplication            |
                                CreateProcessFlag_PoolPartitionMask        |
                                CreateProcessFlag_OptimizeMemoryAllocation |
                                CreateProcessFlag_DisableDeviceAddressSpaceMerge,
    };

    /* Debug types. */
    enum DebugEvent : u32 {
        DebugEvent_CreateProcess = 0,
        DebugEvent_CreateThread  = 1,
        DebugEvent_ExitProcess   = 2,
        DebugEvent_ExitThread    = 3,
        DebugEvent_Exception     = 4,
    };

    enum DebugThreadParam : u32 {
        DebugThreadParam_Priority     = 0,
        DebugThreadParam_State        = 1,
        DebugThreadParam_IdealCore    = 2,
        DebugThreadParam_CurrentCore  = 3,
        DebugThreadParam_AffinityMask = 4,
    };

    enum DebugException : u32 {
        DebugException_UndefinedInstruction = 0,
        DebugException_InstructionAbort     = 1,
        DebugException_DataAbort            = 2,
        DebugException_AlignmentFault       = 3,
        DebugException_DebuggerAttached     = 4,
        DebugException_BreakPoint           = 5,
        DebugException_UserBreak            = 6,
        DebugException_DebuggerBreak        = 7,
        DebugException_UndefinedSystemCall  = 8,
        DebugException_MemorySystemError    = 9,
    };

    enum DebugEventFlag : u32 {
        DebugEventFlag_Stopped = (1u << 0),
    };

    enum ExceptionType : u32 {
        ExceptionType_Init                 = 0x000,
        ExceptionType_InstructionAbort     = 0x100,
        ExceptionType_DataAbort            = 0x101,
        ExceptionType_UnalignedInstruction = 0x102,
        ExceptionType_UnalignedData        = 0x103,
        ExceptionType_UndefinedInstruction = 0x104,
        ExceptionType_ExceptionInstruction = 0x105,
        ExceptionType_MemorySystemError    = 0x106,
        ExceptionType_FpuException         = 0x200,
        ExceptionType_InvalidSystemCall    = 0x301,
        ExceptionType_SystemCallBreak      = 0x302,

        ExceptionType_AtmosphereStdAbort   = 0xFFE,
    };

    enum BreakReason : u32 {
        BreakReason_Panic         = 0,
        BreakReason_Assert        = 1,
        BreakReason_User          = 2,
        BreakReason_PreLoadDll    = 3,
        BreakReason_PostLoadDll   = 4,
        BreakReason_PreUnloadDll  = 5,
        BreakReason_PostUnloadDll = 6,
        BreakReason_CppException  = 7,

        BreakReason_NotificationOnlyFlag = 0x80000000,
    };

    enum KernelDebugType : u32 {
        KernelDebugType_Thread          =  0,
        KernelDebugType_ThreadCallStack =  1,
        KernelDebugType_KernelObject    =  2,
        KernelDebugType_Handle          =  3,
        KernelDebugType_Memory          =  4,
        KernelDebugType_PageTable       =  5,
        KernelDebugType_CpuUtilization  =  6,
        KernelDebugType_Process         =  7,
        KernelDebugType_SuspendProcess  =  8,
        KernelDebugType_ResumeProcess   =  9,
        KernelDebugType_Port            = 10,
    };

    enum KernelTraceState : u32 {
        KernelTraceState_Disabled = 0,
        KernelTraceState_Enabled  = 1,
    };

    enum BreakPointType : u32 {
        BreakPointType_HardwareInstruction = 0,
        BreakPointType_HardwareData        = 1,
    };

    enum HardwareBreakPointRegisterName : u32 {
        HardwareBreakPointRegisterName_I0  = 0,
        HardwareBreakPointRegisterName_I1  = 1,
        HardwareBreakPointRegisterName_I2  = 2,
        HardwareBreakPointRegisterName_I3  = 3,
        HardwareBreakPointRegisterName_I4  = 4,
        HardwareBreakPointRegisterName_I5  = 5,
        HardwareBreakPointRegisterName_I6  = 6,
        HardwareBreakPointRegisterName_I7  = 7,
        HardwareBreakPointRegisterName_I8  = 8,
        HardwareBreakPointRegisterName_I9  = 9,
        HardwareBreakPointRegisterName_I10 = 10,
        HardwareBreakPointRegisterName_I11 = 11,
        HardwareBreakPointRegisterName_I12 = 12,
        HardwareBreakPointRegisterName_I13 = 13,
        HardwareBreakPointRegisterName_I14 = 14,
        HardwareBreakPointRegisterName_I15 = 15,
        HardwareBreakPointRegisterName_D0  = 16,
        HardwareBreakPointRegisterName_D1  = 17,
        HardwareBreakPointRegisterName_D2  = 18,
        HardwareBreakPointRegisterName_D3  = 19,
        HardwareBreakPointRegisterName_D4  = 20,
        HardwareBreakPointRegisterName_D5  = 21,
        HardwareBreakPointRegisterName_D6  = 22,
        HardwareBreakPointRegisterName_D7  = 23,
        HardwareBreakPointRegisterName_D8  = 24,
        HardwareBreakPointRegisterName_D9  = 25,
        HardwareBreakPointRegisterName_D10 = 26,
        HardwareBreakPointRegisterName_D11 = 27,
        HardwareBreakPointRegisterName_D12 = 28,
        HardwareBreakPointRegisterName_D13 = 29,
        HardwareBreakPointRegisterName_D14 = 30,
        HardwareBreakPointRegisterName_D15 = 31,
    };

    /* Architecture specific types. */
    namespace aarch64 {

        struct ExceptionInfo {
            u64 r[9];
            u64 lr;
            u64 sp;
            u64 pc;
            u32 pstate;
            u32 afsr0;
            u32 afsr1;
            u32 esr;
            u64 far;
        };
        static_assert(sizeof(ExceptionInfo) == 0x78);

        struct ProcessLocalRegion {
            u64           data[(0x1C0 - sizeof(ExceptionInfo)) / sizeof(u64)];
            ExceptionInfo exception_info;
            u64           dying_message_region_address;
            u64           dying_message_region_size;
            u64           padding[6];
        };
        static_assert(sizeof(ProcessLocalRegion) == 0x200);
        static_assert(OFFSETOF(ProcessLocalRegion, dying_message_region_address) == 0x1C0);

    }

    namespace aarch32 {

        struct ExceptionInfoStatus32 {
            u32 cpsr;
            u32 fsr;
            u32 far;
            u32 fpexc;
            u32 fpinst;
            u32 fpinst2;
        };

        struct ExceptionInfoStatus64 {
            u32 pstate;
            u32 afsr0;
            u32 afsr1;
            u32 esr;
            u32 far;
        };

        struct ExceptionInfo {
            u32 r[8];
            u32 sp;
            u32 lr;
            u32 pc;
            u32 flags;
            union {
                ExceptionInfoStatus32 status_32;
                ExceptionInfoStatus64 status_64;
            };
        };
        static_assert(sizeof(ExceptionInfo) == 0x48);

        struct ProcessLocalRegion {
            u32           data[(0x1C0 - sizeof(ExceptionInfo)) / sizeof(u32)];
            ExceptionInfo exception_info;
            u64           dying_message_region_address;
            u64           dying_message_region_size;
            u64           padding[6];
        };
        static_assert(sizeof(ProcessLocalRegion) == 0x200);
        static_assert(OFFSETOF(ProcessLocalRegion, dying_message_region_address) == 0x1C0);

    }

    /* Secure monitor argument shims. */
    namespace lp64 {

        struct SecureMonitorArguments {
            u64 r[8];
        };
        static_assert(sizeof(SecureMonitorArguments) == 0x40);

    }

    namespace ilp32 {

        struct SecureMonitorArguments {
            u32 r[8];
        };
        static_assert(sizeof(SecureMonitorArguments) == 0x20);

    }

}
