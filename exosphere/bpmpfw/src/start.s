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
    msr cpsr_f, #0xC0
    msr cpsr_cf, #0xD3
    ldr sp, =__stack_top__
    ldr lr, =reboot
    bl lp0_entry_main
    infloop:
        b infloop
       
       
.global spinlock_wait
.type spinlock_wait, %function
spinlock_wait:
    sub r0, r0, #1
    cmp r0, #0
    bgt spinlock_wait
    bx lr