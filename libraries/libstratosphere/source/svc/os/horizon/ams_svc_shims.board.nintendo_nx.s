.section .text._ZN3ams3svc7aarch644lp6412CreateIoPoolEPjNS0_5board8nintendo2nx10IoPoolTypeE, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6412CreateIoPoolEPjNS0_5board8nintendo2nx10IoPoolTypeE
.type _ZN3ams3svc7aarch644lp6412CreateIoPoolEPjNS0_5board8nintendo2nx10IoPoolTypeE, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6412CreateIoPoolEPjNS0_5board8nintendo2nx10IoPoolTypeE:
    str x0, [sp, #-16]!
    svc 0x39
    ldr x2, [sp], #16
    str w1, [x2]
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6424AttachDeviceAddressSpaceENS0_5board8nintendo2nx10DeviceNameEj, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6424AttachDeviceAddressSpaceENS0_5board8nintendo2nx10DeviceNameEj
.type _ZN3ams3svc7aarch644lp6424AttachDeviceAddressSpaceENS0_5board8nintendo2nx10DeviceNameEj, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6424AttachDeviceAddressSpaceENS0_5board8nintendo2nx10DeviceNameEj:
    svc 0x57
    ret
.cfi_endproc

.section .text._ZN3ams3svc7aarch644lp6424DetachDeviceAddressSpaceENS0_5board8nintendo2nx10DeviceNameEj, "ax", %progbits
.global _ZN3ams3svc7aarch644lp6424DetachDeviceAddressSpaceENS0_5board8nintendo2nx10DeviceNameEj
.type _ZN3ams3svc7aarch644lp6424DetachDeviceAddressSpaceENS0_5board8nintendo2nx10DeviceNameEj, %function
.align 2
.cfi_startproc
_ZN3ams3svc7aarch644lp6424DetachDeviceAddressSpaceENS0_5board8nintendo2nx10DeviceNameEj:
    svc 0x58
    ret
.cfi_endproc
