.section .text.start
.align 4
.global _start
_start:
    b crt0
.global _reboot
    b reboot

.global crt0
.type crt0, %function
crt0:
    @ setup to call lp0_entry_main
    msr cpsr_cxsf, #0xD3
    ldr sp, =__stack_top__
    ldr lr, =reboot
    b lp0_entry_main


.global spinlock_wait
.type spinlock_wait, %function
spinlock_wait:
    subs r0, r0, #1
    bgt spinlock_wait
    bx lr
