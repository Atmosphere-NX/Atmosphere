/*
 * Copyright (c) Atmosphère-NX
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
#include <mesosphere/kern_select_assembly_offsets.h>

#define SAVE_THREAD_CONTEXT(ctx, tmp0, tmp1, done_label)            \
    /* Save the callee save registers + SP and cpacr. */            \
    mov tmp0, sp;                                                   \
    mrs tmp1, cpacr_el1;                                            \
    stp  x19,  x20, [ctx, #(THREAD_CONTEXT_X19_X20)];               \
    stp  x21,  x22, [ctx, #(THREAD_CONTEXT_X21_X22)];               \
    stp  x23,  x24, [ctx, #(THREAD_CONTEXT_X23_X24)];               \
    stp  x25,  x26, [ctx, #(THREAD_CONTEXT_X25_X26)];               \
    stp  x27,  x28, [ctx, #(THREAD_CONTEXT_X27_X28)];               \
    stp  x29,  x30, [ctx, #(THREAD_CONTEXT_X29_X30)];               \
                                                                    \
    stp tmp0, tmp1, [ctx, #(THREAD_CONTEXT_SP_CPACR)];              \
                                                                    \
    /* Check whether the FPU is enabled. */                         \
    /* If it isn't, skip saving FPU state. */                       \
    and tmp1, tmp1, #0x300000;                                      \
    cbz tmp1, done_label;                                           \
                                                                    \
    /* Save fpcr and fpsr. */                                       \
    mrs tmp0, fpcr;                                                 \
    mrs tmp1, fpsr;                                                 \
    stp tmp0, tmp1, [ctx, #(THREAD_CONTEXT_FPCR_FPSR)];             \
                                                                    \
    /* Save the FPU registers. */                                   \
    stp q0,  q1,  [ctx, #(16 *  0 + THREAD_CONTEXT_FPU_REGISTERS)]; \
    stp q2,  q3,  [ctx, #(16 *  2 + THREAD_CONTEXT_FPU_REGISTERS)]; \
    stp q4,  q5,  [ctx, #(16 *  4 + THREAD_CONTEXT_FPU_REGISTERS)]; \
    stp q6,  q7,  [ctx, #(16 *  6 + THREAD_CONTEXT_FPU_REGISTERS)]; \
    stp q8,  q9,  [ctx, #(16 *  8 + THREAD_CONTEXT_FPU_REGISTERS)]; \
    stp q10, q11, [ctx, #(16 * 10 + THREAD_CONTEXT_FPU_REGISTERS)]; \
    stp q12, q13, [ctx, #(16 * 12 + THREAD_CONTEXT_FPU_REGISTERS)]; \
    stp q14, q15, [ctx, #(16 * 14 + THREAD_CONTEXT_FPU_REGISTERS)]; \
    stp q16, q17, [ctx, #(16 * 16 + THREAD_CONTEXT_FPU_REGISTERS)]; \
    stp q18, q19, [ctx, #(16 * 18 + THREAD_CONTEXT_FPU_REGISTERS)]; \
    stp q20, q21, [ctx, #(16 * 20 + THREAD_CONTEXT_FPU_REGISTERS)]; \
    stp q22, q23, [ctx, #(16 * 22 + THREAD_CONTEXT_FPU_REGISTERS)]; \
    stp q24, q25, [ctx, #(16 * 24 + THREAD_CONTEXT_FPU_REGISTERS)]; \
    stp q26, q27, [ctx, #(16 * 26 + THREAD_CONTEXT_FPU_REGISTERS)]; \
    stp q28, q29, [ctx, #(16 * 28 + THREAD_CONTEXT_FPU_REGISTERS)]; \
    stp q30, q31, [ctx, #(16 * 30 + THREAD_CONTEXT_FPU_REGISTERS)];

#define RESTORE_THREAD_CONTEXT(ctx, tmp0, tmp1, done_label)         \
    /* Restore the callee save registers + SP and cpacr. */         \
    ldp tmp0, tmp1, [ctx, #(THREAD_CONTEXT_SP_CPACR)];              \
    mov sp, tmp0;                                                   \
    ldp  x19,  x20, [ctx, #(THREAD_CONTEXT_X19_X20)];               \
    ldp  x21,  x22, [ctx, #(THREAD_CONTEXT_X21_X22)];               \
    ldp  x23,  x24, [ctx, #(THREAD_CONTEXT_X23_X24)];               \
    ldp  x25,  x26, [ctx, #(THREAD_CONTEXT_X25_X26)];               \
    ldp  x27,  x28, [ctx, #(THREAD_CONTEXT_X27_X28)];               \
    ldp  x29,  x30, [ctx, #(THREAD_CONTEXT_X29_X30)];               \
                                                                    \
    msr cpacr_el1, tmp1;                                            \
    isb;                                                            \
                                                                    \
    /* Check whether the FPU is enabled. */                         \
    /* If it isn't, skip saving FPU state. */                       \
    and tmp1, tmp1, #0x300000;                                      \
    cbz tmp1, done_label;                                           \
                                                                    \
    /* Save fpcr and fpsr. */                                       \
    ldp tmp0, tmp1, [ctx, #(THREAD_CONTEXT_FPCR_FPSR)];             \
    msr fpcr, tmp0;                                                 \
    msr fpsr, tmp1;                                                 \
                                                                    \
    /* Save the FPU registers. */                                   \
    ldp q0,  q1,  [ctx, #(16 *  0 + THREAD_CONTEXT_FPU_REGISTERS)]; \
    ldp q2,  q3,  [ctx, #(16 *  2 + THREAD_CONTEXT_FPU_REGISTERS)]; \
    ldp q4,  q5,  [ctx, #(16 *  4 + THREAD_CONTEXT_FPU_REGISTERS)]; \
    ldp q6,  q7,  [ctx, #(16 *  6 + THREAD_CONTEXT_FPU_REGISTERS)]; \
    ldp q8,  q9,  [ctx, #(16 *  8 + THREAD_CONTEXT_FPU_REGISTERS)]; \
    ldp q10, q11, [ctx, #(16 * 10 + THREAD_CONTEXT_FPU_REGISTERS)]; \
    ldp q12, q13, [ctx, #(16 * 12 + THREAD_CONTEXT_FPU_REGISTERS)]; \
    ldp q14, q15, [ctx, #(16 * 14 + THREAD_CONTEXT_FPU_REGISTERS)]; \
    ldp q16, q17, [ctx, #(16 * 16 + THREAD_CONTEXT_FPU_REGISTERS)]; \
    ldp q18, q19, [ctx, #(16 * 18 + THREAD_CONTEXT_FPU_REGISTERS)]; \
    ldp q20, q21, [ctx, #(16 * 20 + THREAD_CONTEXT_FPU_REGISTERS)]; \
    ldp q22, q23, [ctx, #(16 * 22 + THREAD_CONTEXT_FPU_REGISTERS)]; \
    ldp q24, q25, [ctx, #(16 * 24 + THREAD_CONTEXT_FPU_REGISTERS)]; \
    ldp q26, q27, [ctx, #(16 * 26 + THREAD_CONTEXT_FPU_REGISTERS)]; \
    ldp q28, q29, [ctx, #(16 * 28 + THREAD_CONTEXT_FPU_REGISTERS)]; \
    ldp q30, q31, [ctx, #(16 * 30 + THREAD_CONTEXT_FPU_REGISTERS)];


/* ams::kern::KScheduler::ScheduleImpl() */
.section    .text._ZN3ams4kern10KScheduler12ScheduleImplEv, "ax", %progbits
.global     _ZN3ams4kern10KScheduler12ScheduleImplEv
.type       _ZN3ams4kern10KScheduler12ScheduleImplEv, %function

/* Ensure ScheduleImpl is aligned to 0x40 bytes. */
.balign 0x40

_ZN3ams4kern10KScheduler12ScheduleImplEv:
    /* Right now, x0 contains (this). We want x1 to point to the scheduling state, */
    /* KScheduler layout has state at +0x0, this is guaranteed statically by assembly offsets. */
    mov    x1, x0

    /* First, clear the need's scheduling bool (and dmb ish after, as it's an atomic). */
    /* TODO: Should this be a stlrb? Nintendo does not do one. */
    strb   wzr, [x1]
    dmb    ish

    /* Check whether there are runnable interrupt tasks. */
    ldrb   w8, [x1, #(KSCHEDULER_INTERRUPT_TASK_RUNNABLE)]
    cbnz   w8, 0f

    /* If it isn't, we want to check if the highest priority thread is the same as the current thread. */
    ldr    x7, [x1, #(KSCHEDULER_HIGHEST_PRIORITY_THREAD)]
    cmp    x7, x18
    b.ne   1f

    /* If they're the same, then we can just issue a memory barrier and return. */
    dmb    ish
    ret

0:  /* The interrupt task thread is runnable. */
    /* We want to switch to the interrupt task/idle thread. */
    mov    x7, #0

1:  /* The highest priority thread is not the same as the current thread. */
    /* Get a reference to the current thread's stack parameters. */
    add    x2, sp, #0x1000
    and    x2, x2, #~(0x1000-1)

    /* Check if the thread has terminated. We can do this by checking the DPC flags for DpcFlag_Terminated. */
    ldurb  w3, [x2, #-(THREAD_STACK_PARAMETERS_SIZE - THREAD_STACK_PARAMETERS_DPC_FLAGS)]
    tbnz   w3, #1, 3f

    /* The current thread hasn't terminated, so we want to save its context. */
    ldur  x2, [x2, #-(THREAD_STACK_PARAMETERS_SIZE - THREAD_STACK_PARAMETERS_CONTEXT)]
    SAVE_THREAD_CONTEXT(x2, x4, x5, 2f)

2:  /* We're done saving this thread's context, so we need to unlock it. */
    /* We can just do an atomic write to the relevant KThreadContext member. */
    add    x2, x2, #(THREAD_CONTEXT_LOCKED)
    stlrb  wzr, [x2]

3:  /* The current thread's context has been entirely taken care of. */
    /* Now we want to loop until we successfully switch the thread context. */
    /* Start by saving all the values we care about in callee-save registers. */
    mov    x19, x0 /* this */
    mov    x20, x1 /* this->state */
    mov    x21, x7 /* highest priority thread */

    /* Set our stack to the idle thread stack. */
    ldr    x3, [x20, #(KSCHEDULER_IDLE_THREAD_STACK)]
    mov    sp, x3
    b      5f

4:  /* We failed to successfully do the context switch, and need to retry. */
    /* Clear the exclusive monitor. */
    clrex

    /* Clear the need's scheduling bool (and dmb ish after, as it's an atomic). */
    /* TODO: Should this be a stlrb? Nintendo does not do one. */
    strb   wzr, [x20]
    dmb    ish

    /* Refresh the highest priority thread. */
    ldr    x21, [x20, #(KSCHEDULER_HIGHEST_PRIORITY_THREAD)]

5:  /* We're starting to try to do the context switch. */
    /* Check if the highest priority thread if null. */
    /* If it is, we want to branch to a special idle thread loop. */
    cbz    x21, 11f

    /* Get the highest priority thread's context, and save it. */
    /* ams::kern::KThread::GetContextForSchedulerLoop() */
    add    x22, x21, #(THREAD_THREAD_CONTEXT)

    /* Prepare to try to acquire the context lock. */
    add    x1, x22, #(THREAD_CONTEXT_LOCKED)
    mov    w2, #1

6:  /* We want to try to lock the highest priority thread's context. */
    /* Check if the lock is already held. */
    ldaxrb w3, [x1]
    cbnz   w3, 7f

    /* If it's not, try to take it. */
    stxrb  w3, w2, [x1]
    cbnz   w3, 6b

    /* We hold the lock, so we can now switch the thread. */
    b      8f

7:  /* The highest priority thread's context is already locked. */
    /* Check if we need scheduling. If we don't, we can retry directly. */
    ldarb  w3, [x20] // ldarb w3, [x20, #(KSCHEDULER_NEEDS_SCHEDULING)]
    cbz    w3, 6b

    /* If we do, another core is interfering, and we must start from the top. */
    b      4b

8:  /* It's time to switch the thread. */
    /* Switch to the highest priority thread. */
    mov    x0, x19
    mov    x1, x21

    /* Call ams::kern::KScheduler::SwitchThread(ams::kern::KThread *) */
    bl     _ZN3ams4kern10KScheduler12SwitchThreadEPNS0_7KThreadE

    /* Check if we need scheduling. If we don't, then we can't complete the switch and should retry. */
    ldarb  w1, [x20] // ldarb w1, [x20, #(KSCHEDULER_NEEDS_SCHEDULING)]
    cbnz   w1, 10f

    /* Restore the thread context. */
    mov    x0, x22
    RESTORE_THREAD_CONTEXT(x0, x1, x2, 9f)

9:  /* We're done restoring the thread context, and can return safely. */
    ret

10: /* Our switch failed. */
    /* We should unlock the thread context, and then retry. */
    add    x1, x22, #(THREAD_CONTEXT_LOCKED)
    stlrb  wzr, [x1]
    b      4b

11: /* The next thread is nullptr! */
    /* Switch to nullptr. This will actually switch to the idle thread. */
    mov    x0, x19
    mov    x1, #0

    /* Call ams::kern::KScheduler::SwitchThread(ams::kern::KThread *) */
    bl     _ZN3ams4kern10KScheduler12SwitchThreadEPNS0_7KThreadE

12: /* We've switched to the idle thread, so we want to process interrupt tasks until we schedule a non-idle thread. */
    /* Check whether there are runnable interrupt tasks. */
    ldrb   w3, [x20, #(KSCHEDULER_INTERRUPT_TASK_RUNNABLE)]
    cbnz   w3, 13f

    /* Check if we need scheduling. */
    ldarb  w3, [x20] // ldarb w3, [x20, #(KSCHEDULER_NEEDS_SCHEDULING)]
    cbnz   w3, 4b

    /* Clear the previous thread. */
    str    xzr, [x20, #(KSCHEDULER_PREVIOUS_THREAD)]

    /* Wait for an interrupt and check again. */
    wfi

    msr    daifclr, #2
    msr    daifset, #2

    b      12b

13: /* We have interrupt tasks to execute! */
    /* Execute any pending interrupt tasks. */
    ldr    x0, [x20, #(KSCHEDULER_INTERRUPT_TASK_MANAGER)]
    bl     _ZN3ams4kern21KInterruptTaskManager7DoTasksEv

    /* Clear the interrupt task thread as runnable. */
    strb   wzr, [x20, #(KSCHEDULER_INTERRUPT_TASK_RUNNABLE)]

    /* Retry the scheduling loop. */
    b      4b
