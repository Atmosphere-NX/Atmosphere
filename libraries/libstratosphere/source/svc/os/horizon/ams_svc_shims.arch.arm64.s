.section .text._ZN3ams3svc7aarch644lp6411SetHeapSizeEPNS0_7AddressENS0_4SizeE, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6411SetHeapSizeEPNS0_7AddressENS0_4SizeE
.type _ZN3ams3svc7aarch644lp6411SetHeapSizeEPNS0_7AddressENS0_4SizeE, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6411SetHeapSizeEPNS0_7AddressENS0_4SizeE:
    str x0, [sp, #-16]!
    svc 0x1
    ldr x2, [sp], #16
    str x1, [x2]
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6419SetMemoryPermissionENS0_7AddressENS0_4SizeENS0_16MemoryPermissionE, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6419SetMemoryPermissionENS0_7AddressENS0_4SizeENS0_16MemoryPermissionE
.type _ZN3ams3svc7aarch644lp6419SetMemoryPermissionENS0_7AddressENS0_4SizeENS0_16MemoryPermissionE, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6419SetMemoryPermissionENS0_7AddressENS0_4SizeENS0_16MemoryPermissionE:
    svc 0x2
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6418SetMemoryAttributeENS0_7AddressENS0_4SizeEjj, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6418SetMemoryAttributeENS0_7AddressENS0_4SizeEjj
.type _ZN3ams3svc7aarch644lp6418SetMemoryAttributeENS0_7AddressENS0_4SizeEjj, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6418SetMemoryAttributeENS0_7AddressENS0_4SizeEjj:
    svc 0x3
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp649MapMemoryENS0_7AddressES3_NS0_4SizeE, "ax", %progbits
.global _ZN3ams3svc7aarch644lp649MapMemoryENS0_7AddressES3_NS0_4SizeE
.type _ZN3ams3svc7aarch644lp649MapMemoryENS0_7AddressES3_NS0_4SizeE, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp649MapMemoryENS0_7AddressES3_NS0_4SizeE:
    svc 0x4
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6411UnmapMemoryENS0_7AddressES3_NS0_4SizeE, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6411UnmapMemoryENS0_7AddressES3_NS0_4SizeE
.type _ZN3ams3svc7aarch644lp6411UnmapMemoryENS0_7AddressES3_NS0_4SizeE, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6411UnmapMemoryENS0_7AddressES3_NS0_4SizeE:
    svc 0x5
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6411QueryMemoryENS0_11UserPointerIPNS0_4lp6410MemoryInfoEEEPNS0_8PageInfoENS0_7AddressE, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6411QueryMemoryENS0_11UserPointerIPNS0_4lp6410MemoryInfoEEEPNS0_8PageInfoENS0_7AddressE
.type _ZN3ams3svc7aarch644lp6411QueryMemoryENS0_11UserPointerIPNS0_4lp6410MemoryInfoEEEPNS0_8PageInfoENS0_7AddressE, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6411QueryMemoryENS0_11UserPointerIPNS0_4lp6410MemoryInfoEEEPNS0_8PageInfoENS0_7AddressE:
    str x1, [sp, #-16]!
    svc 0x6
    ldr x2, [sp], #16
    str w1, [x2]
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6411ExitProcessEv, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6411ExitProcessEv
.type _ZN3ams3svc7aarch644lp6411ExitProcessEv, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6411ExitProcessEv:
    svc 0x7
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6412CreateThreadEPjNS0_7AddressES4_S4_ii, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6412CreateThreadEPjNS0_7AddressES4_S4_ii
.type _ZN3ams3svc7aarch644lp6412CreateThreadEPjNS0_7AddressES4_S4_ii, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6412CreateThreadEPjNS0_7AddressES4_S4_ii:
    str x0, [sp, #-16]!
    svc 0x8
    ldr x2, [sp], #16
    str w1, [x2]
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6411StartThreadEj, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6411StartThreadEj
.type _ZN3ams3svc7aarch644lp6411StartThreadEj, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6411StartThreadEj:
    svc 0x9
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6410ExitThreadEv, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6410ExitThreadEv
.type _ZN3ams3svc7aarch644lp6410ExitThreadEv, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6410ExitThreadEv:
    svc 0xA
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6411SleepThreadEl, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6411SleepThreadEl
.type _ZN3ams3svc7aarch644lp6411SleepThreadEl, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6411SleepThreadEl:
    svc 0xB
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6417GetThreadPriorityEPij, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6417GetThreadPriorityEPij
.type _ZN3ams3svc7aarch644lp6417GetThreadPriorityEPij, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6417GetThreadPriorityEPij:
    str x0, [sp, #-16]!
    svc 0xC
    ldr x2, [sp], #16
    str w1, [x2]
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6417SetThreadPriorityEji, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6417SetThreadPriorityEji
.type _ZN3ams3svc7aarch644lp6417SetThreadPriorityEji, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6417SetThreadPriorityEji:
    svc 0xD
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6417GetThreadCoreMaskEPiPmj, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6417GetThreadCoreMaskEPiPmj
.type _ZN3ams3svc7aarch644lp6417GetThreadCoreMaskEPiPmj, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6417GetThreadCoreMaskEPiPmj:
    stp x0, x1, [sp, #-16]!
    svc 0xE
    ldp x3, x4, [sp], #16
    str w1, [x3]
    str x2, [x4]
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6417SetThreadCoreMaskEjim, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6417SetThreadCoreMaskEjim
.type _ZN3ams3svc7aarch644lp6417SetThreadCoreMaskEjim, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6417SetThreadCoreMaskEjim:
    svc 0xF
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6425GetCurrentProcessorNumberEv, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6425GetCurrentProcessorNumberEv
.type _ZN3ams3svc7aarch644lp6425GetCurrentProcessorNumberEv, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6425GetCurrentProcessorNumberEv:
    svc 0x10
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6411SignalEventEj, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6411SignalEventEj
.type _ZN3ams3svc7aarch644lp6411SignalEventEj, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6411SignalEventEj:
    svc 0x11
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6410ClearEventEj, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6410ClearEventEj
.type _ZN3ams3svc7aarch644lp6410ClearEventEj, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6410ClearEventEj:
    svc 0x12
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6415MapSharedMemoryEjNS0_7AddressENS0_4SizeENS0_16MemoryPermissionE, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6415MapSharedMemoryEjNS0_7AddressENS0_4SizeENS0_16MemoryPermissionE
.type _ZN3ams3svc7aarch644lp6415MapSharedMemoryEjNS0_7AddressENS0_4SizeENS0_16MemoryPermissionE, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6415MapSharedMemoryEjNS0_7AddressENS0_4SizeENS0_16MemoryPermissionE:
    svc 0x13
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6417UnmapSharedMemoryEjNS0_7AddressENS0_4SizeE, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6417UnmapSharedMemoryEjNS0_7AddressENS0_4SizeE
.type _ZN3ams3svc7aarch644lp6417UnmapSharedMemoryEjNS0_7AddressENS0_4SizeE, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6417UnmapSharedMemoryEjNS0_7AddressENS0_4SizeE:
    svc 0x14
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6420CreateTransferMemoryEPjNS0_7AddressENS0_4SizeENS0_16MemoryPermissionE, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6420CreateTransferMemoryEPjNS0_7AddressENS0_4SizeENS0_16MemoryPermissionE
.type _ZN3ams3svc7aarch644lp6420CreateTransferMemoryEPjNS0_7AddressENS0_4SizeENS0_16MemoryPermissionE, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6420CreateTransferMemoryEPjNS0_7AddressENS0_4SizeENS0_16MemoryPermissionE:
    str x0, [sp, #-16]!
    svc 0x15
    ldr x2, [sp], #16
    str w1, [x2]
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6411CloseHandleEj, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6411CloseHandleEj
.type _ZN3ams3svc7aarch644lp6411CloseHandleEj, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6411CloseHandleEj:
    svc 0x16
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6411ResetSignalEj, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6411ResetSignalEj
.type _ZN3ams3svc7aarch644lp6411ResetSignalEj, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6411ResetSignalEj:
    svc 0x17
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6419WaitSynchronizationEPiNS0_11UserPointerIPKjEEil, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6419WaitSynchronizationEPiNS0_11UserPointerIPKjEEil
.type _ZN3ams3svc7aarch644lp6419WaitSynchronizationEPiNS0_11UserPointerIPKjEEil, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6419WaitSynchronizationEPiNS0_11UserPointerIPKjEEil:
    str x0, [sp, #-16]!
    svc 0x18
    ldr x2, [sp], #16
    str w1, [x2]
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6421CancelSynchronizationEj, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6421CancelSynchronizationEj
.type _ZN3ams3svc7aarch644lp6421CancelSynchronizationEj, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6421CancelSynchronizationEj:
    svc 0x19
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6413ArbitrateLockEjNS0_7AddressEj, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6413ArbitrateLockEjNS0_7AddressEj
.type _ZN3ams3svc7aarch644lp6413ArbitrateLockEjNS0_7AddressEj, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6413ArbitrateLockEjNS0_7AddressEj:
    svc 0x1A
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6415ArbitrateUnlockENS0_7AddressE, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6415ArbitrateUnlockENS0_7AddressE
.type _ZN3ams3svc7aarch644lp6415ArbitrateUnlockENS0_7AddressE, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6415ArbitrateUnlockENS0_7AddressE:
    svc 0x1B
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6424WaitProcessWideKeyAtomicENS0_7AddressES3_jl, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6424WaitProcessWideKeyAtomicENS0_7AddressES3_jl
.type _ZN3ams3svc7aarch644lp6424WaitProcessWideKeyAtomicENS0_7AddressES3_jl, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6424WaitProcessWideKeyAtomicENS0_7AddressES3_jl:
    svc 0x1C
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6420SignalProcessWideKeyENS0_7AddressEi, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6420SignalProcessWideKeyENS0_7AddressEi
.type _ZN3ams3svc7aarch644lp6420SignalProcessWideKeyENS0_7AddressEi, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6420SignalProcessWideKeyENS0_7AddressEi:
    svc 0x1D
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6413GetSystemTickEv, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6413GetSystemTickEv
.type _ZN3ams3svc7aarch644lp6413GetSystemTickEv, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6413GetSystemTickEv:
    svc 0x1E
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6418ConnectToNamedPortEPjNS0_11UserPointerIPKcEE, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6418ConnectToNamedPortEPjNS0_11UserPointerIPKcEE
.type _ZN3ams3svc7aarch644lp6418ConnectToNamedPortEPjNS0_11UserPointerIPKcEE, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6418ConnectToNamedPortEPjNS0_11UserPointerIPKcEE:
    str x0, [sp, #-16]!
    svc 0x1F
    ldr x2, [sp], #16
    str w1, [x2]
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6420SendSyncRequestLightEj, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6420SendSyncRequestLightEj
.type _ZN3ams3svc7aarch644lp6420SendSyncRequestLightEj, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6420SendSyncRequestLightEj:
    svc 0x20
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6415SendSyncRequestEj, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6415SendSyncRequestEj
.type _ZN3ams3svc7aarch644lp6415SendSyncRequestEj, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6415SendSyncRequestEj:
    svc 0x21
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6429SendSyncRequestWithUserBufferENS0_7AddressENS0_4SizeEj, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6429SendSyncRequestWithUserBufferENS0_7AddressENS0_4SizeEj
.type _ZN3ams3svc7aarch644lp6429SendSyncRequestWithUserBufferENS0_7AddressENS0_4SizeEj, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6429SendSyncRequestWithUserBufferENS0_7AddressENS0_4SizeEj:
    svc 0x22
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6430SendAsyncRequestWithUserBufferEPjNS0_7AddressENS0_4SizeEj, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6430SendAsyncRequestWithUserBufferEPjNS0_7AddressENS0_4SizeEj
.type _ZN3ams3svc7aarch644lp6430SendAsyncRequestWithUserBufferEPjNS0_7AddressENS0_4SizeEj, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6430SendAsyncRequestWithUserBufferEPjNS0_7AddressENS0_4SizeEj:
    str x0, [sp, #-16]!
    svc 0x23
    ldr x2, [sp], #16
    str w1, [x2]
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6412GetProcessIdEPmj, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6412GetProcessIdEPmj
.type _ZN3ams3svc7aarch644lp6412GetProcessIdEPmj, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6412GetProcessIdEPmj:
    str x0, [sp, #-16]!
    svc 0x24
    ldr x2, [sp], #16
    str x1, [x2]
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6411GetThreadIdEPmj, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6411GetThreadIdEPmj
.type _ZN3ams3svc7aarch644lp6411GetThreadIdEPmj, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6411GetThreadIdEPmj:
    str x0, [sp, #-16]!
    svc 0x25
    ldr x2, [sp], #16
    str x1, [x2]
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp645BreakENS0_11BreakReasonENS0_7AddressENS0_4SizeE, "ax", %progbits
.global _ZN3ams3svc7aarch644lp645BreakENS0_11BreakReasonENS0_7AddressENS0_4SizeE
.type _ZN3ams3svc7aarch644lp645BreakENS0_11BreakReasonENS0_7AddressENS0_4SizeE, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp645BreakENS0_11BreakReasonENS0_7AddressENS0_4SizeE:
    svc 0x26
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6417OutputDebugStringENS0_11UserPointerIPKcEENS0_4SizeE, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6417OutputDebugStringENS0_11UserPointerIPKcEENS0_4SizeE
.type _ZN3ams3svc7aarch644lp6417OutputDebugStringENS0_11UserPointerIPKcEENS0_4SizeE, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6417OutputDebugStringENS0_11UserPointerIPKcEENS0_4SizeE:
    svc 0x27
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6419ReturnFromExceptionENS_6ResultE, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6419ReturnFromExceptionENS_6ResultE
.type _ZN3ams3svc7aarch644lp6419ReturnFromExceptionENS_6ResultE, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6419ReturnFromExceptionENS_6ResultE:
    svc 0x28
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp647GetInfoEPmNS0_8InfoTypeEjm, "ax", %progbits
.global _ZN3ams3svc7aarch644lp647GetInfoEPmNS0_8InfoTypeEjm
.type _ZN3ams3svc7aarch644lp647GetInfoEPmNS0_8InfoTypeEjm, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp647GetInfoEPmNS0_8InfoTypeEjm:
    str x0, [sp, #-16]!
    svc 0x29
    ldr x2, [sp], #16
    str x1, [x2]
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6420FlushEntireDataCacheEv, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6420FlushEntireDataCacheEv
.type _ZN3ams3svc7aarch644lp6420FlushEntireDataCacheEv, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6420FlushEntireDataCacheEv:
    svc 0x2A
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6414FlushDataCacheENS0_7AddressENS0_4SizeE, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6414FlushDataCacheENS0_7AddressENS0_4SizeE
.type _ZN3ams3svc7aarch644lp6414FlushDataCacheENS0_7AddressENS0_4SizeE, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6414FlushDataCacheENS0_7AddressENS0_4SizeE:
    svc 0x2B
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6417MapPhysicalMemoryENS0_7AddressENS0_4SizeE, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6417MapPhysicalMemoryENS0_7AddressENS0_4SizeE
.type _ZN3ams3svc7aarch644lp6417MapPhysicalMemoryENS0_7AddressENS0_4SizeE, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6417MapPhysicalMemoryENS0_7AddressENS0_4SizeE:
    svc 0x2C
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6419UnmapPhysicalMemoryENS0_7AddressENS0_4SizeE, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6419UnmapPhysicalMemoryENS0_7AddressENS0_4SizeE
.type _ZN3ams3svc7aarch644lp6419UnmapPhysicalMemoryENS0_7AddressENS0_4SizeE, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6419UnmapPhysicalMemoryENS0_7AddressENS0_4SizeE:
    svc 0x2D
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6424GetDebugFutureThreadInfoEPNS0_4lp6417LastThreadContextEPmjl, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6424GetDebugFutureThreadInfoEPNS0_4lp6417LastThreadContextEPmjl
.type _ZN3ams3svc7aarch644lp6424GetDebugFutureThreadInfoEPNS0_4lp6417LastThreadContextEPmjl, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6424GetDebugFutureThreadInfoEPNS0_4lp6417LastThreadContextEPmjl:
    stp x0, x1, [sp, #-16]!
    svc 0x2E
    ldp x6, x7, [sp], #16
    stp x1, x2, [x6]
    stp x3, x4, [x6, #16]
    str x5, [x7]
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6417GetLastThreadInfoEPNS0_4lp6417LastThreadContextEPNS0_7AddressEPj, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6417GetLastThreadInfoEPNS0_4lp6417LastThreadContextEPNS0_7AddressEPj
.type _ZN3ams3svc7aarch644lp6417GetLastThreadInfoEPNS0_4lp6417LastThreadContextEPNS0_7AddressEPj, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6417GetLastThreadInfoEPNS0_4lp6417LastThreadContextEPNS0_7AddressEPj:
    stp x1, x2, [sp, #-16]!
    str x0, [sp, #-16]!
    svc 0x2F
    ldr x7, [sp], #16
    stp x1, x2, [x7]
    stp x3, x4, [x7, #16]
    ldp x1, x2, [sp], #16
    str x5, [x1]
    str w6, [x2]
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6426GetResourceLimitLimitValueEPljNS0_17LimitableResourceE, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6426GetResourceLimitLimitValueEPljNS0_17LimitableResourceE
.type _ZN3ams3svc7aarch644lp6426GetResourceLimitLimitValueEPljNS0_17LimitableResourceE, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6426GetResourceLimitLimitValueEPljNS0_17LimitableResourceE:
    str x0, [sp, #-16]!
    svc 0x30
    ldr x2, [sp], #16
    str x1, [x2]
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6428GetResourceLimitCurrentValueEPljNS0_17LimitableResourceE, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6428GetResourceLimitCurrentValueEPljNS0_17LimitableResourceE
.type _ZN3ams3svc7aarch644lp6428GetResourceLimitCurrentValueEPljNS0_17LimitableResourceE, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6428GetResourceLimitCurrentValueEPljNS0_17LimitableResourceE:
    str x0, [sp, #-16]!
    svc 0x31
    ldr x2, [sp], #16
    str x1, [x2]
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6417SetThreadActivityEjNS0_14ThreadActivityE, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6417SetThreadActivityEjNS0_14ThreadActivityE
.type _ZN3ams3svc7aarch644lp6417SetThreadActivityEjNS0_14ThreadActivityE, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6417SetThreadActivityEjNS0_14ThreadActivityE:
    svc 0x32
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6417GetThreadContext3ENS0_11UserPointerIPNS0_13ThreadContextEEEj, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6417GetThreadContext3ENS0_11UserPointerIPNS0_13ThreadContextEEEj
.type _ZN3ams3svc7aarch644lp6417GetThreadContext3ENS0_11UserPointerIPNS0_13ThreadContextEEEj, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6417GetThreadContext3ENS0_11UserPointerIPNS0_13ThreadContextEEEj:
    svc 0x33
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6414WaitForAddressENS0_7AddressENS0_15ArbitrationTypeEil, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6414WaitForAddressENS0_7AddressENS0_15ArbitrationTypeEil
.type _ZN3ams3svc7aarch644lp6414WaitForAddressENS0_7AddressENS0_15ArbitrationTypeEil, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6414WaitForAddressENS0_7AddressENS0_15ArbitrationTypeEil:
    svc 0x34
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6415SignalToAddressENS0_7AddressENS0_10SignalTypeEii, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6415SignalToAddressENS0_7AddressENS0_10SignalTypeEii
.type _ZN3ams3svc7aarch644lp6415SignalToAddressENS0_7AddressENS0_10SignalTypeEii, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6415SignalToAddressENS0_7AddressENS0_10SignalTypeEii:
    svc 0x35
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6426SynchronizePreemptionStateEv, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6426SynchronizePreemptionStateEv
.type _ZN3ams3svc7aarch644lp6426SynchronizePreemptionStateEv, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6426SynchronizePreemptionStateEv:
    svc 0x36
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6425GetResourceLimitPeakValueEPljNS0_17LimitableResourceE, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6425GetResourceLimitPeakValueEPljNS0_17LimitableResourceE
.type _ZN3ams3svc7aarch644lp6425GetResourceLimitPeakValueEPljNS0_17LimitableResourceE, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6425GetResourceLimitPeakValueEPljNS0_17LimitableResourceE:
    str x0, [sp, #-16]!
    svc 0x37
    ldr x2, [sp], #16
    str x1, [x2]
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6414CreateIoRegionEPjjmNS0_4SizeENS0_13MemoryMappingENS0_16MemoryPermissionE, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6414CreateIoRegionEPjjmNS0_4SizeENS0_13MemoryMappingENS0_16MemoryPermissionE
.type _ZN3ams3svc7aarch644lp6414CreateIoRegionEPjjmNS0_4SizeENS0_13MemoryMappingENS0_16MemoryPermissionE, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6414CreateIoRegionEPjjmNS0_4SizeENS0_13MemoryMappingENS0_16MemoryPermissionE:
    str x0, [sp, #-16]!
    svc 0x3A
    ldr x2, [sp], #16
    str w1, [x2]
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6411KernelDebugENS0_15KernelDebugTypeEmmm, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6411KernelDebugENS0_15KernelDebugTypeEmmm
.type _ZN3ams3svc7aarch644lp6411KernelDebugENS0_15KernelDebugTypeEmmm, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6411KernelDebugENS0_15KernelDebugTypeEmmm:
    svc 0x3C
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6422ChangeKernelTraceStateENS0_16KernelTraceStateE, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6422ChangeKernelTraceStateENS0_16KernelTraceStateE
.type _ZN3ams3svc7aarch644lp6422ChangeKernelTraceStateENS0_16KernelTraceStateE, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6422ChangeKernelTraceStateENS0_16KernelTraceStateE:
    svc 0x3D
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6413CreateSessionEPjS3_bNS0_7AddressE, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6413CreateSessionEPjS3_bNS0_7AddressE
.type _ZN3ams3svc7aarch644lp6413CreateSessionEPjS3_bNS0_7AddressE, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6413CreateSessionEPjS3_bNS0_7AddressE:
    stp x0, x1, [sp, #-16]!
    svc 0x40
    ldp x3, x4, [sp], #16
    str w1, [x3]
    str w2, [x4]
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6413AcceptSessionEPjj, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6413AcceptSessionEPjj
.type _ZN3ams3svc7aarch644lp6413AcceptSessionEPjj, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6413AcceptSessionEPjj:
    str x0, [sp, #-16]!
    svc 0x41
    ldr x2, [sp], #16
    str w1, [x2]
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6420ReplyAndReceiveLightEj, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6420ReplyAndReceiveLightEj
.type _ZN3ams3svc7aarch644lp6420ReplyAndReceiveLightEj, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6420ReplyAndReceiveLightEj:
    svc 0x42
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6415ReplyAndReceiveEPiNS0_11UserPointerIPKjEEijl, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6415ReplyAndReceiveEPiNS0_11UserPointerIPKjEEijl
.type _ZN3ams3svc7aarch644lp6415ReplyAndReceiveEPiNS0_11UserPointerIPKjEEijl, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6415ReplyAndReceiveEPiNS0_11UserPointerIPKjEEijl:
    str x0, [sp, #-16]!
    svc 0x43
    ldr x2, [sp], #16
    str w1, [x2]
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6429ReplyAndReceiveWithUserBufferEPiNS0_7AddressENS0_4SizeENS0_11UserPointerIPKjEEijl, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6429ReplyAndReceiveWithUserBufferEPiNS0_7AddressENS0_4SizeENS0_11UserPointerIPKjEEijl
.type _ZN3ams3svc7aarch644lp6429ReplyAndReceiveWithUserBufferEPiNS0_7AddressENS0_4SizeENS0_11UserPointerIPKjEEijl, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6429ReplyAndReceiveWithUserBufferEPiNS0_7AddressENS0_4SizeENS0_11UserPointerIPKjEEijl:
    str x0, [sp, #-16]!
    svc 0x44
    ldr x2, [sp], #16
    str w1, [x2]
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6411CreateEventEPjS3_, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6411CreateEventEPjS3_
.type _ZN3ams3svc7aarch644lp6411CreateEventEPjS3_, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6411CreateEventEPjS3_:
    stp x0, x1, [sp, #-16]!
    svc 0x45
    ldp x3, x4, [sp], #16
    str w1, [x3]
    str w2, [x4]
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6411MapIoRegionEjNS0_7AddressENS0_4SizeENS0_16MemoryPermissionE, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6411MapIoRegionEjNS0_7AddressENS0_4SizeENS0_16MemoryPermissionE
.type _ZN3ams3svc7aarch644lp6411MapIoRegionEjNS0_7AddressENS0_4SizeENS0_16MemoryPermissionE, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6411MapIoRegionEjNS0_7AddressENS0_4SizeENS0_16MemoryPermissionE:
    svc 0x46
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6413UnmapIoRegionEjNS0_7AddressENS0_4SizeE, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6413UnmapIoRegionEjNS0_7AddressENS0_4SizeE
.type _ZN3ams3svc7aarch644lp6413UnmapIoRegionEjNS0_7AddressENS0_4SizeE, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6413UnmapIoRegionEjNS0_7AddressENS0_4SizeE:
    svc 0x47
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6423MapPhysicalMemoryUnsafeENS0_7AddressENS0_4SizeE, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6423MapPhysicalMemoryUnsafeENS0_7AddressENS0_4SizeE
.type _ZN3ams3svc7aarch644lp6423MapPhysicalMemoryUnsafeENS0_7AddressENS0_4SizeE, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6423MapPhysicalMemoryUnsafeENS0_7AddressENS0_4SizeE:
    svc 0x48
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6425UnmapPhysicalMemoryUnsafeENS0_7AddressENS0_4SizeE, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6425UnmapPhysicalMemoryUnsafeENS0_7AddressENS0_4SizeE
.type _ZN3ams3svc7aarch644lp6425UnmapPhysicalMemoryUnsafeENS0_7AddressENS0_4SizeE, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6425UnmapPhysicalMemoryUnsafeENS0_7AddressENS0_4SizeE:
    svc 0x49
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6414SetUnsafeLimitENS0_4SizeE, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6414SetUnsafeLimitENS0_4SizeE
.type _ZN3ams3svc7aarch644lp6414SetUnsafeLimitENS0_4SizeE, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6414SetUnsafeLimitENS0_4SizeE:
    svc 0x4A
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6416CreateCodeMemoryEPjNS0_7AddressENS0_4SizeE, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6416CreateCodeMemoryEPjNS0_7AddressENS0_4SizeE
.type _ZN3ams3svc7aarch644lp6416CreateCodeMemoryEPjNS0_7AddressENS0_4SizeE, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6416CreateCodeMemoryEPjNS0_7AddressENS0_4SizeE:
    str x0, [sp, #-16]!
    svc 0x4B
    ldr x2, [sp], #16
    str w1, [x2]
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6417ControlCodeMemoryEjNS0_19CodeMemoryOperationEmmNS0_16MemoryPermissionE, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6417ControlCodeMemoryEjNS0_19CodeMemoryOperationEmmNS0_16MemoryPermissionE
.type _ZN3ams3svc7aarch644lp6417ControlCodeMemoryEjNS0_19CodeMemoryOperationEmmNS0_16MemoryPermissionE, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6417ControlCodeMemoryEjNS0_19CodeMemoryOperationEmmNS0_16MemoryPermissionE:
    svc 0x4C
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6411SleepSystemEv, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6411SleepSystemEv
.type _ZN3ams3svc7aarch644lp6411SleepSystemEv, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6411SleepSystemEv:
    svc 0x4D
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6417ReadWriteRegisterEPjmjj, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6417ReadWriteRegisterEPjmjj
.type _ZN3ams3svc7aarch644lp6417ReadWriteRegisterEPjmjj, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6417ReadWriteRegisterEPjmjj:
    str x0, [sp, #-16]!
    svc 0x4E
    ldr x2, [sp], #16
    str w1, [x2]
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6418SetProcessActivityEjNS0_15ProcessActivityE, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6418SetProcessActivityEjNS0_15ProcessActivityE
.type _ZN3ams3svc7aarch644lp6418SetProcessActivityEjNS0_15ProcessActivityE, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6418SetProcessActivityEjNS0_15ProcessActivityE:
    svc 0x4F
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6418CreateSharedMemoryEPjNS0_4SizeENS0_16MemoryPermissionES5_, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6418CreateSharedMemoryEPjNS0_4SizeENS0_16MemoryPermissionES5_
.type _ZN3ams3svc7aarch644lp6418CreateSharedMemoryEPjNS0_4SizeENS0_16MemoryPermissionES5_, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6418CreateSharedMemoryEPjNS0_4SizeENS0_16MemoryPermissionES5_:
    str x0, [sp, #-16]!
    svc 0x50
    ldr x2, [sp], #16
    str w1, [x2]
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6417MapTransferMemoryEjNS0_7AddressENS0_4SizeENS0_16MemoryPermissionE, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6417MapTransferMemoryEjNS0_7AddressENS0_4SizeENS0_16MemoryPermissionE
.type _ZN3ams3svc7aarch644lp6417MapTransferMemoryEjNS0_7AddressENS0_4SizeENS0_16MemoryPermissionE, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6417MapTransferMemoryEjNS0_7AddressENS0_4SizeENS0_16MemoryPermissionE:
    svc 0x51
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6419UnmapTransferMemoryEjNS0_7AddressENS0_4SizeE, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6419UnmapTransferMemoryEjNS0_7AddressENS0_4SizeE
.type _ZN3ams3svc7aarch644lp6419UnmapTransferMemoryEjNS0_7AddressENS0_4SizeE, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6419UnmapTransferMemoryEjNS0_7AddressENS0_4SizeE:
    svc 0x52
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6420CreateInterruptEventEPjiNS0_13InterruptTypeE, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6420CreateInterruptEventEPjiNS0_13InterruptTypeE
.type _ZN3ams3svc7aarch644lp6420CreateInterruptEventEPjiNS0_13InterruptTypeE, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6420CreateInterruptEventEPjiNS0_13InterruptTypeE:
    str x0, [sp, #-16]!
    svc 0x53
    ldr x2, [sp], #16
    str w1, [x2]
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6420QueryPhysicalAddressEPNS0_4lp6418PhysicalMemoryInfoENS0_7AddressE, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6420QueryPhysicalAddressEPNS0_4lp6418PhysicalMemoryInfoENS0_7AddressE
.type _ZN3ams3svc7aarch644lp6420QueryPhysicalAddressEPNS0_4lp6418PhysicalMemoryInfoENS0_7AddressE, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6420QueryPhysicalAddressEPNS0_4lp6418PhysicalMemoryInfoENS0_7AddressE:
    str x0, [sp, #-16]!
    svc 0x54
    ldr x4, [sp], #16
    stp x1, x2, [x4]
    str x3, [x4, #16]
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6414QueryIoMappingEPNS0_7AddressEPNS0_4SizeEmS5_, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6414QueryIoMappingEPNS0_7AddressEPNS0_4SizeEmS5_
.type _ZN3ams3svc7aarch644lp6414QueryIoMappingEPNS0_7AddressEPNS0_4SizeEmS5_, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6414QueryIoMappingEPNS0_7AddressEPNS0_4SizeEmS5_:
    stp x0, x1, [sp, #-16]!
    svc 0x55
    ldp x3, x4, [sp], #16
    str x1, [x3]
    str x2, [x4]
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6420LegacyQueryIoMappingEPNS0_7AddressEmNS0_4SizeE, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6420LegacyQueryIoMappingEPNS0_7AddressEmNS0_4SizeE
.type _ZN3ams3svc7aarch644lp6420LegacyQueryIoMappingEPNS0_7AddressEmNS0_4SizeE, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6420LegacyQueryIoMappingEPNS0_7AddressEmNS0_4SizeE:
    str x0, [sp, #-16]!
    svc 0x55
    ldr x2, [sp], #16
    str x1, [x2]
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6424CreateDeviceAddressSpaceEPjmm, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6424CreateDeviceAddressSpaceEPjmm
.type _ZN3ams3svc7aarch644lp6424CreateDeviceAddressSpaceEPjmm, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6424CreateDeviceAddressSpaceEPjmm:
    str x0, [sp, #-16]!
    svc 0x56
    ldr x2, [sp], #16
    str w1, [x2]
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6428MapDeviceAddressSpaceByForceEjjmNS0_4SizeEmNS0_16MemoryPermissionE, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6428MapDeviceAddressSpaceByForceEjjmNS0_4SizeEmNS0_16MemoryPermissionE
.type _ZN3ams3svc7aarch644lp6428MapDeviceAddressSpaceByForceEjjmNS0_4SizeEmNS0_16MemoryPermissionE, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6428MapDeviceAddressSpaceByForceEjjmNS0_4SizeEmNS0_16MemoryPermissionE:
    svc 0x59
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6428MapDeviceAddressSpaceAlignedEjjmNS0_4SizeEmNS0_16MemoryPermissionE, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6428MapDeviceAddressSpaceAlignedEjjmNS0_4SizeEmNS0_16MemoryPermissionE
.type _ZN3ams3svc7aarch644lp6428MapDeviceAddressSpaceAlignedEjjmNS0_4SizeEmNS0_16MemoryPermissionE, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6428MapDeviceAddressSpaceAlignedEjjmNS0_4SizeEmNS0_16MemoryPermissionE:
    svc 0x5A
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6423UnmapDeviceAddressSpaceEjjmNS0_4SizeEm, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6423UnmapDeviceAddressSpaceEjjmNS0_4SizeEm
.type _ZN3ams3svc7aarch644lp6423UnmapDeviceAddressSpaceEjjmNS0_4SizeEm, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6423UnmapDeviceAddressSpaceEjjmNS0_4SizeEm:
    svc 0x5C
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6426InvalidateProcessDataCacheEjmm, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6426InvalidateProcessDataCacheEjmm
.type _ZN3ams3svc7aarch644lp6426InvalidateProcessDataCacheEjmm, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6426InvalidateProcessDataCacheEjmm:
    svc 0x5D
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6421StoreProcessDataCacheEjmm, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6421StoreProcessDataCacheEjmm
.type _ZN3ams3svc7aarch644lp6421StoreProcessDataCacheEjmm, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6421StoreProcessDataCacheEjmm:
    svc 0x5E
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6421FlushProcessDataCacheEjmm, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6421FlushProcessDataCacheEjmm
.type _ZN3ams3svc7aarch644lp6421FlushProcessDataCacheEjmm, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6421FlushProcessDataCacheEjmm:
    svc 0x5F
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6418DebugActiveProcessEPjm, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6418DebugActiveProcessEPjm
.type _ZN3ams3svc7aarch644lp6418DebugActiveProcessEPjm, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6418DebugActiveProcessEPjm:
    str x0, [sp, #-16]!
    svc 0x60
    ldr x2, [sp], #16
    str w1, [x2]
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6417BreakDebugProcessEj, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6417BreakDebugProcessEj
.type _ZN3ams3svc7aarch644lp6417BreakDebugProcessEj, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6417BreakDebugProcessEj:
    svc 0x61
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6421TerminateDebugProcessEj, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6421TerminateDebugProcessEj
.type _ZN3ams3svc7aarch644lp6421TerminateDebugProcessEj, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6421TerminateDebugProcessEj:
    svc 0x62
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6413GetDebugEventENS0_11UserPointerIPNS0_4lp6414DebugEventInfoEEEj, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6413GetDebugEventENS0_11UserPointerIPNS0_4lp6414DebugEventInfoEEEj
.type _ZN3ams3svc7aarch644lp6413GetDebugEventENS0_11UserPointerIPNS0_4lp6414DebugEventInfoEEEj, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6413GetDebugEventENS0_11UserPointerIPNS0_4lp6414DebugEventInfoEEEj:
    svc 0x63
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6418ContinueDebugEventEjjNS0_11UserPointerIPKmEEi, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6418ContinueDebugEventEjjNS0_11UserPointerIPKmEEi
.type _ZN3ams3svc7aarch644lp6418ContinueDebugEventEjjNS0_11UserPointerIPKmEEi, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6418ContinueDebugEventEjjNS0_11UserPointerIPKmEEi:
    svc 0x64
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6424LegacyContinueDebugEventEjjm, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6424LegacyContinueDebugEventEjjm
.type _ZN3ams3svc7aarch644lp6424LegacyContinueDebugEventEjjm, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6424LegacyContinueDebugEventEjjm:
    svc 0x64
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6414GetProcessListEPiNS0_11UserPointerIPmEEi, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6414GetProcessListEPiNS0_11UserPointerIPmEEi
.type _ZN3ams3svc7aarch644lp6414GetProcessListEPiNS0_11UserPointerIPmEEi, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6414GetProcessListEPiNS0_11UserPointerIPmEEi:
    str x0, [sp, #-16]!
    svc 0x65
    ldr x2, [sp], #16
    str w1, [x2]
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6413GetThreadListEPiNS0_11UserPointerIPmEEij, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6413GetThreadListEPiNS0_11UserPointerIPmEEij
.type _ZN3ams3svc7aarch644lp6413GetThreadListEPiNS0_11UserPointerIPmEEij, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6413GetThreadListEPiNS0_11UserPointerIPmEEij:
    str x0, [sp, #-16]!
    svc 0x66
    ldr x2, [sp], #16
    str w1, [x2]
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6421GetDebugThreadContextENS0_11UserPointerIPNS0_13ThreadContextEEEjmj, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6421GetDebugThreadContextENS0_11UserPointerIPNS0_13ThreadContextEEEjmj
.type _ZN3ams3svc7aarch644lp6421GetDebugThreadContextENS0_11UserPointerIPNS0_13ThreadContextEEEjmj, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6421GetDebugThreadContextENS0_11UserPointerIPNS0_13ThreadContextEEEjmj:
    svc 0x67
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6421SetDebugThreadContextEjmNS0_11UserPointerIPKNS0_13ThreadContextEEEj, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6421SetDebugThreadContextEjmNS0_11UserPointerIPKNS0_13ThreadContextEEEj
.type _ZN3ams3svc7aarch644lp6421SetDebugThreadContextEjmNS0_11UserPointerIPKNS0_13ThreadContextEEEj, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6421SetDebugThreadContextEjmNS0_11UserPointerIPKNS0_13ThreadContextEEEj:
    svc 0x68
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6423QueryDebugProcessMemoryENS0_11UserPointerIPNS0_4lp6410MemoryInfoEEEPNS0_8PageInfoEjNS0_7AddressE, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6423QueryDebugProcessMemoryENS0_11UserPointerIPNS0_4lp6410MemoryInfoEEEPNS0_8PageInfoEjNS0_7AddressE
.type _ZN3ams3svc7aarch644lp6423QueryDebugProcessMemoryENS0_11UserPointerIPNS0_4lp6410MemoryInfoEEEPNS0_8PageInfoEjNS0_7AddressE, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6423QueryDebugProcessMemoryENS0_11UserPointerIPNS0_4lp6410MemoryInfoEEEPNS0_8PageInfoEjNS0_7AddressE:
    str x1, [sp, #-16]!
    svc 0x69
    ldr x2, [sp], #16
    str w1, [x2]
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6422ReadDebugProcessMemoryENS0_7AddressEjS3_NS0_4SizeE, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6422ReadDebugProcessMemoryENS0_7AddressEjS3_NS0_4SizeE
.type _ZN3ams3svc7aarch644lp6422ReadDebugProcessMemoryENS0_7AddressEjS3_NS0_4SizeE, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6422ReadDebugProcessMemoryENS0_7AddressEjS3_NS0_4SizeE:
    svc 0x6A
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6423WriteDebugProcessMemoryEjNS0_7AddressES3_NS0_4SizeE, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6423WriteDebugProcessMemoryEjNS0_7AddressES3_NS0_4SizeE
.type _ZN3ams3svc7aarch644lp6423WriteDebugProcessMemoryEjNS0_7AddressES3_NS0_4SizeE, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6423WriteDebugProcessMemoryEjNS0_7AddressES3_NS0_4SizeE:
    svc 0x6B
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6421SetHardwareBreakPointENS0_30HardwareBreakPointRegisterNameEmm, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6421SetHardwareBreakPointENS0_30HardwareBreakPointRegisterNameEmm
.type _ZN3ams3svc7aarch644lp6421SetHardwareBreakPointENS0_30HardwareBreakPointRegisterNameEmm, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6421SetHardwareBreakPointENS0_30HardwareBreakPointRegisterNameEmm:
    svc 0x6C
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6419GetDebugThreadParamEPmPjjmNS0_16DebugThreadParamE, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6419GetDebugThreadParamEPmPjjmNS0_16DebugThreadParamE
.type _ZN3ams3svc7aarch644lp6419GetDebugThreadParamEPmPjjmNS0_16DebugThreadParamE, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6419GetDebugThreadParamEPmPjjmNS0_16DebugThreadParamE:
    stp x0, x1, [sp, #-16]!
    svc 0x6D
    ldp x3, x4, [sp], #16
    str x1, [x3]
    str w2, [x4]
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6413GetSystemInfoEPmNS0_14SystemInfoTypeEjm, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6413GetSystemInfoEPmNS0_14SystemInfoTypeEjm
.type _ZN3ams3svc7aarch644lp6413GetSystemInfoEPmNS0_14SystemInfoTypeEjm, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6413GetSystemInfoEPmNS0_14SystemInfoTypeEjm:
    str x0, [sp, #-16]!
    svc 0x6F
    ldr x2, [sp], #16
    str x1, [x2]
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6410CreatePortEPjS3_ibNS0_7AddressE, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6410CreatePortEPjS3_ibNS0_7AddressE
.type _ZN3ams3svc7aarch644lp6410CreatePortEPjS3_ibNS0_7AddressE, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6410CreatePortEPjS3_ibNS0_7AddressE:
    stp x0, x1, [sp, #-16]!
    svc 0x70
    ldp x3, x4, [sp], #16
    str w1, [x3]
    str w2, [x4]
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6415ManageNamedPortEPjNS0_11UserPointerIPKcEEi, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6415ManageNamedPortEPjNS0_11UserPointerIPKcEEi
.type _ZN3ams3svc7aarch644lp6415ManageNamedPortEPjNS0_11UserPointerIPKcEEi, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6415ManageNamedPortEPjNS0_11UserPointerIPKcEEi:
    str x0, [sp, #-16]!
    svc 0x71
    ldr x2, [sp], #16
    str w1, [x2]
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6413ConnectToPortEPjj, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6413ConnectToPortEPjj
.type _ZN3ams3svc7aarch644lp6413ConnectToPortEPjj, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6413ConnectToPortEPjj:
    str x0, [sp, #-16]!
    svc 0x72
    ldr x2, [sp], #16
    str w1, [x2]
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6426SetProcessMemoryPermissionEjmmNS0_16MemoryPermissionE, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6426SetProcessMemoryPermissionEjmmNS0_16MemoryPermissionE
.type _ZN3ams3svc7aarch644lp6426SetProcessMemoryPermissionEjmmNS0_16MemoryPermissionE, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6426SetProcessMemoryPermissionEjmmNS0_16MemoryPermissionE:
    svc 0x73
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6416MapProcessMemoryENS0_7AddressEjmNS0_4SizeE, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6416MapProcessMemoryENS0_7AddressEjmNS0_4SizeE
.type _ZN3ams3svc7aarch644lp6416MapProcessMemoryENS0_7AddressEjmNS0_4SizeE, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6416MapProcessMemoryENS0_7AddressEjmNS0_4SizeE:
    svc 0x74
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6418UnmapProcessMemoryENS0_7AddressEjmNS0_4SizeE, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6418UnmapProcessMemoryENS0_7AddressEjmNS0_4SizeE
.type _ZN3ams3svc7aarch644lp6418UnmapProcessMemoryENS0_7AddressEjmNS0_4SizeE, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6418UnmapProcessMemoryENS0_7AddressEjmNS0_4SizeE:
    svc 0x75
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6418QueryProcessMemoryENS0_11UserPointerIPNS0_4lp6410MemoryInfoEEEPNS0_8PageInfoEjm, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6418QueryProcessMemoryENS0_11UserPointerIPNS0_4lp6410MemoryInfoEEEPNS0_8PageInfoEjm
.type _ZN3ams3svc7aarch644lp6418QueryProcessMemoryENS0_11UserPointerIPNS0_4lp6410MemoryInfoEEEPNS0_8PageInfoEjm, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6418QueryProcessMemoryENS0_11UserPointerIPNS0_4lp6410MemoryInfoEEEPNS0_8PageInfoEjm:
    str x1, [sp, #-16]!
    svc 0x76
    ldr x2, [sp], #16
    str w1, [x2]
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6420MapProcessCodeMemoryEjmmm, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6420MapProcessCodeMemoryEjmmm
.type _ZN3ams3svc7aarch644lp6420MapProcessCodeMemoryEjmmm, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6420MapProcessCodeMemoryEjmmm:
    svc 0x77
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6422UnmapProcessCodeMemoryEjmmm, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6422UnmapProcessCodeMemoryEjmmm
.type _ZN3ams3svc7aarch644lp6422UnmapProcessCodeMemoryEjmmm, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6422UnmapProcessCodeMemoryEjmmm:
    svc 0x78
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6413CreateProcessEPjNS0_11UserPointerIPKNS0_4lp6422CreateProcessParameterEEENS4_IPKjEEi, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6413CreateProcessEPjNS0_11UserPointerIPKNS0_4lp6422CreateProcessParameterEEENS4_IPKjEEi
.type _ZN3ams3svc7aarch644lp6413CreateProcessEPjNS0_11UserPointerIPKNS0_4lp6422CreateProcessParameterEEENS4_IPKjEEi, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6413CreateProcessEPjNS0_11UserPointerIPKNS0_4lp6422CreateProcessParameterEEENS4_IPKjEEi:
    str x0, [sp, #-16]!
    svc 0x79
    ldr x2, [sp], #16
    str w1, [x2]
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6412StartProcessEjiim, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6412StartProcessEjiim
.type _ZN3ams3svc7aarch644lp6412StartProcessEjiim, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6412StartProcessEjiim:
    svc 0x7A
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6416TerminateProcessEj, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6416TerminateProcessEj
.type _ZN3ams3svc7aarch644lp6416TerminateProcessEj, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6416TerminateProcessEj:
    svc 0x7B
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6414GetProcessInfoEPljNS0_15ProcessInfoTypeE, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6414GetProcessInfoEPljNS0_15ProcessInfoTypeE
.type _ZN3ams3svc7aarch644lp6414GetProcessInfoEPljNS0_15ProcessInfoTypeE, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6414GetProcessInfoEPljNS0_15ProcessInfoTypeE:
    str x0, [sp, #-16]!
    svc 0x7C
    ldr x2, [sp], #16
    str w1, [x2]
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6419CreateResourceLimitEPj, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6419CreateResourceLimitEPj
.type _ZN3ams3svc7aarch644lp6419CreateResourceLimitEPj, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6419CreateResourceLimitEPj:
    str x0, [sp, #-16]!
    svc 0x7D
    ldr x2, [sp], #16
    str w1, [x2]
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6426SetResourceLimitLimitValueEjNS0_17LimitableResourceEl, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6426SetResourceLimitLimitValueEjNS0_17LimitableResourceEl
.type _ZN3ams3svc7aarch644lp6426SetResourceLimitLimitValueEjNS0_17LimitableResourceEl, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6426SetResourceLimitLimitValueEjNS0_17LimitableResourceEl:
    svc 0x7E
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6417CallSecureMonitorEPNS0_4lp6422SecureMonitorArgumentsE, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6417CallSecureMonitorEPNS0_4lp6422SecureMonitorArgumentsE
.type _ZN3ams3svc7aarch644lp6417CallSecureMonitorEPNS0_4lp6422SecureMonitorArgumentsE, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6417CallSecureMonitorEPNS0_4lp6422SecureMonitorArgumentsE:
    str x0, [sp, #-16]!
    mov x8, x0
    ldp x0, x1, [x8]
    ldp x2, x3, [x8, #0x10]
    ldp x4, x5, [x8, #0x20]
    ldp x6, x7, [x8, #0x30]
    svc 0x7F
    ldr x8, [sp], #16
    stp x0, x1, [x8]
    stp x2, x3, [x8, #0x10]
    stp x4, x5, [x8, #0x20]
    stp x6, x7, [x8, #0x30]
    ret
.cfi_endproc
