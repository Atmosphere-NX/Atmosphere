.align      4
.section    .text.cold.start, "ax", %progbits
.global     __start_cold
__start_cold:
    /* mask all interrupts */
    msr daifset, daif

    /*
        Enable invalidates of branch target buffer, then flush
        the entire instruction cache at the local level, and
        with the reg change, the branch target buffer, then disable
        invalidates of the branch target buffer again.
    */
    mrs  x0, cpuactlr_el1
    orr  x0, x0, #1
    msr  cpuactlr_el1, x0

    dsb  sy
    isb
    ic   iallu
    dsb  sy
    isb

    mrs  x0, cpuactlr_el1
    bic  x0, x0, #1
    msr  cpuactlr_el1, x0

    /* if the OS lock is set, disable it and request a warm reset */
    mrs  x0, oslsr_el1
    ands x0, x0, #2
    b.eq _set_lock_and_sp
    mov  x0, xzr
    msr  oslar_el1, x0

    mov  x0, #(1 << 63)
    msr  cpuactlr_el1, x0 /* disable regional clock gating */
    isb
    mov  x0, #3
    msr  rmr_el3, x0
    isb
    dsb
    wfi

    _set_lock_and_sp:
    /* set the OS lock */
    mov  x0, #1
    msr  oslar_el1, x0

    /* set SP = SP_EL0 (temporary stack) */
    msr  spsel, #0
    ldr  x20, =__cold_crt0_stack_top__
    mov  sp, x20
    bl   configure_memory
    ldr  x16, =__init_cold
    br   x16

.section    .text.cold, "ax", %progbits
__init_cold:
    /* set SP = SP_EL3 (exception stack) */
    msr  spsel, #1
    ldr  x20, =__main_stack_top__
    mov  sp, x20

    /* set SP = SP_EL0 (temporary stack) */
    msr  spsel, #0
    ldr  x20, =__pk2_load_stack_top__
    mov  sp, x20
    bl   loadPk2
    ldr  x20, =__cold_init_stack_top__
    mov  sp, x20
    b    coldboot_main

.global     __set_sp_el0_and_jump_to_el1
.type       __set_sp_el0_and_jump_to_el1, %function
__set_sp_el0_and_jump_to_el1:
    /* the official handler does some weird stuff with SP_EL0 */
    msr  elr_el3, x1
    mov  sp, x2

    mov  x1, #0x3c5  /* EL1, all interrupts masked */
    msr  spsr_el3, x1
    isb
    eret
