.macro CLEAR_GPR_REG_ITER
    mov r\@, #0
.endm

.section .text.start
.arm
.align 5
.global _start
_start:
    /* Insert NOPs for convenience (i.e. to use Nintendo's BCTs, for example) */
    .rept 16
    nop
    .endr
    /* Switch to supervisor mode, mask all interrupts, clear all flags */
    msr cpsr_cxsf, #0xDF

    /* Relocate ourselves if necessary */
    ldr r2, =__start__
    adr r3, _start
    cmp r2, r3
    bne _relocation_loop_end

    ldr r4, =__bss_start__
    sub r4, r4, r2          /* size >= 32, obviously */
    _relocation_loop:
        ldmia r3!, {r5-r12}
        stmia r2!, {r5-r12}
        subs  r4, #0x20
        bne _relocation_loop

    ldr r12, =_relocation_loop_end
    bx  r12

    _relocation_loop_end:
    /* Set the stack pointer */
    ldr sp, =0x40008000
    mov fp, #0
    stmfd sp!, {r0, r1}

    /* Clear .bss */
    ldr r0, =__bss_start__
    mov r1, #0
    ldr r2, =__bss_end__
    sub r2, r2, r0
    bl  memset

    /* Call global constructors */
    bl  __libc_init_array

    /* Set r0 to r12 to 0 (because why not?) & call main */
    .rept 13
    CLEAR_GPR_REG_ITER
    .endr
    ldmfd sp!, {r0, r1}
    bl  main
    b   .
