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

#define SAVE_THREAD_CONTEXT(ctx, tmp0, tmp1, done_label)    \
    /* Save the callee save registers + SP and cpacr. */    \
    mov tmp0, sp;                                           \
    mrs tmp1, cpacr_el1;                                    \
    stp  x19,  x20, [ctx, #(8 *  0)];                       \
    stp  x21,  x22, [ctx, #(8 *  2)];                       \
    stp  x23,  x24, [ctx, #(8 *  4)];                       \
    stp  x25,  x26, [ctx, #(8 *  6)];                       \
    stp  x27,  x28, [ctx, #(8 *  8)];                       \
    stp  x29,  x30, [ctx, #(8 * 10)];                       \
                                                            \
    stp tmp0, tmp1, [ctx, #0x60];                           \
                                                            \
    /* Check whether the FPU is enabled. */                 \
    /* If it isn't, skip saving FPU state. */               \
    and tmp1, tmp1, #0x300000;                              \
    cbz tmp1, done_label;                                   \
                                                            \
    /* Save fpcr and fpsr. */                               \
    mrs tmp0, fpcr;                                         \
    mrs tmp1, fpsr;                                         \
    stp tmp0, tmp1, [ctx, #0x70];                           \
                                                            \
    /* Save the FPU registers. */                           \
    stp q0,  q1,  [ctx, #(16 *  0 + 0x80)];                 \
    stp q2,  q3,  [ctx, #(16 *  2 + 0x80)];                 \
    stp q4,  q5,  [ctx, #(16 *  4 + 0x80)];                 \
    stp q6,  q7,  [ctx, #(16 *  6 + 0x80)];                 \
    stp q8,  q9,  [ctx, #(16 *  8 + 0x80)];                 \
    stp q10, q11, [ctx, #(16 * 10 + 0x80)];                 \
    stp q12, q13, [ctx, #(16 * 12 + 0x80)];                 \
    stp q14, q15, [ctx, #(16 * 14 + 0x80)];                 \
    stp q16, q17, [ctx, #(16 * 16 + 0x80)];                 \
    stp q18, q19, [ctx, #(16 * 18 + 0x80)];                 \
    stp q20, q21, [ctx, #(16 * 20 + 0x80)];                 \
    stp q22, q23, [ctx, #(16 * 22 + 0x80)];                 \
    stp q24, q25, [ctx, #(16 * 24 + 0x80)];                 \
    stp q26, q27, [ctx, #(16 * 26 + 0x80)];                 \
    stp q28, q29, [ctx, #(16 * 28 + 0x80)];                 \
    stp q30, q31, [ctx, #(16 * 30 + 0x80)];

#define RESTORE_THREAD_CONTEXT(ctx, tmp0, tmp1, done_label) \
    /* Restore the callee save registers + SP and cpacr. */ \
    ldp tmp0, tmp1, [ctx, #0x60];                           \
    mov sp, tmp0;                                           \
    ldp  x19,  x20, [ctx, #(8 *  0)];                       \
    ldp  x21,  x22, [ctx, #(8 *  2)];                       \
    ldp  x23,  x24, [ctx, #(8 *  4)];                       \
    ldp  x25,  x26, [ctx, #(8 *  6)];                       \
    ldp  x27,  x28, [ctx, #(8 *  8)];                       \
    ldp  x29,  x30, [ctx, #(8 * 10)];                       \
                                                            \
    msr cpacr_el1, tmp1;                                    \
    isb;                                                    \
                                                            \
    /* Check whether the FPU is enabled. */                 \
    /* If it isn't, skip saving FPU state. */               \
    and tmp1, tmp1, #0x300000;                              \
    cbz tmp1, done_label;                                   \
                                                            \
    /* Save fpcr and fpsr. */                               \
    ldp tmp0, tmp1, [ctx, #0x70];                           \
    msr fpcr, tmp0;                                         \
    msr fpsr, tmp1;                                         \
                                                            \
    /* Save the FPU registers. */                           \
    ldp q0,  q1,  [ctx, #(16 *  0 + 0x80)];                 \
    ldp q2,  q3,  [ctx, #(16 *  2 + 0x80)];                 \
    ldp q4,  q5,  [ctx, #(16 *  4 + 0x80)];                 \
    ldp q6,  q7,  [ctx, #(16 *  6 + 0x80)];                 \
    ldp q8,  q9,  [ctx, #(16 *  8 + 0x80)];                 \
    ldp q10, q11, [ctx, #(16 * 10 + 0x80)];                 \
    ldp q12, q13, [ctx, #(16 * 12 + 0x80)];                 \
    ldp q14, q15, [ctx, #(16 * 14 + 0x80)];                 \
    ldp q16, q17, [ctx, #(16 * 16 + 0x80)];                 \
    ldp q18, q19, [ctx, #(16 * 18 + 0x80)];                 \
    ldp q20, q21, [ctx, #(16 * 20 + 0x80)];                 \
    ldp q22, q23, [ctx, #(16 * 22 + 0x80)];                 \
    ldp q24, q25, [ctx, #(16 * 24 + 0x80)];                 \
    ldp q26, q27, [ctx, #(16 * 26 + 0x80)];                 \
    ldp q28, q29, [ctx, #(16 * 28 + 0x80)];                 \
    ldp q30, q31, [ctx, #(16 * 30 + 0x80)];


/* ams::kern::KScheduler::ScheduleImpl() */
.section    .text._ZN3ams4kern10KScheduler12ScheduleImplEv, "ax", %progbits
.global     _ZN3ams4kern10KScheduler12ScheduleImplEv
.type       _ZN3ams4kern10KScheduler12ScheduleImplEv, %function

/* Ensure ScheduleImpl is aligned to 0x40 bytes. */
.balign 0x40

_ZN3ams4kern10KScheduler12ScheduleImplEv:
    /* Right now, x0 contains (this). We want x1 to point to the scheduling state, */
    /* Current KScheduler layout has state at +0x0. */
    mov    x1, x0

    /* First thing we want to do is check whether the interrupt task thread is runnable. */
    ldrb   w3, [x1, #1]
    cbz    w3, 0f

    /* If it is, we want to call KScheduler::InterruptTaskThreadToRunnable() to change its state to runnable. */
    stp     x0,  x1, [sp, #-16]!
    stp    x30, xzr, [sp, #-16]!
    bl     _ZN3ams4kern10KScheduler29InterruptTaskThreadToRunnableEv
    ldp    x30, xzr, [sp], 16
    ldp     x0,  x1, [sp], 16

    /* Clear the interrupt task thread as runnable. */
    strb   wzr, [x1, #1]

0:  /* Interrupt task thread runnable checked. */
    /* Now we want to check if there's any scheduling to do. */

    /* First, clear the need's scheduling bool (and dmb ish after, as it's an atomic). */
    /* TODO: Should this be a stlrb? Nintendo does not do one. */
    strb   wzr, [x1]
    dmb    ish

    /* Check if the highest priority thread is the same as the current thread. */
    ldr    x7, [x1, 16]
    ldr    x2, [x18]
    cmp    x7, x2
    b.ne   1f

    /* If they're the same, then we can just return as there's nothing to do. */
    ret

1:  /* The highest priority thread is not the same as the current thread. */
    /* Get a reference to the current thread's stack parameters. */
    add    x2, sp, #0x1000
    and    x2, x2, #~(0x1000-1)

    /* Check if the thread has terminated. We can do this by checking the DPC flags for DpcFlag_Terminated. */
    ldurb  w3, [x2, #-0x20]
    tbnz   w3, #1, 3f

    /* The current thread hasn't terminated, so we want to save its context. */
    ldur  x2, [x2, #-0x10]
    SAVE_THREAD_CONTEXT(x2, x4, x5, 2f)

2:  /* We're done saving this thread's context, so we need to unlock it. */
    /* We can just do an atomic write to the relevant KThreadContext member. */
    add    x2, x2, #0x280
    stlrb  wzr, [x2]

3:  /* The current thread's context has been entirely taken care of. */
    /* Now we want to loop until we successfully switch the thread context. */
    /* Start by saving all the values we care about in callee-save registers. */
    mov    x19, x0 /* this */
    mov    x20, x1 /* this->state */
    mov    x21, x7 /* highest priority thread */

    /* Set our stack to the idle thread stack. */
    ldr    x3, [x20, #0x18]
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
    ldr    x21, [x20, 16]

5:  /* We're starting to try to do the context switch. */
    /* Check if the highest priority thread if null. */
    /* If it is, we want to branch to a special idle thread loop. */
    cbz    x21, 11f

    /* Get the highest priority thread's context, and save it. */
    /* ams::kern::KThread::GetContextForSchedulerLoop() */
    mov    x0, x21
    bl     _ZN3ams4kern7KThread26GetContextForSchedulerLoopEv
    mov    x22, x0

    /* Prepare to try to acquire the context lock. */
    add    x1, x22, #0x280
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
    ldarb  w3, [x20]
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
    ldarb  w1, [x20]
    cbnz   w1, 10f

    /* Restore the thread context. */
    mov    x0, x22
    RESTORE_THREAD_CONTEXT(x0, x1, x2, 9f)

9:  /* We're done restoring the thread context, and can return safely. */
    ret

10: /* Our switch failed. */
    /* We should unlock the thread context, and then retry. */
    add    x1, x22, #0x280
    stlrb  wzr, [x1]
    b      4b

11: /* The next thread is nullptr! */
    /* Switch to nullptr. This will actually switch to the idle thread. */
    mov    x0, x19
    mov    x1, #0

    /* Call ams::kern::KScheduler::SwitchThread(ams::kern::KThread *) */
    bl     _ZN3ams4kern10KScheduler12SwitchThreadEPNS0_7KThreadE

12: /* We've switched to the idle thread, so we want to loop until we schedule a non-idle thread. */
    /* Check if we need scheduling. */
    ldarb  w3, [x20]
    cbnz   w3, 13f

    /* If we don't, wait for an interrupt and check again. */
    wfi

    msr    daifclr, #2
    msr    daifset, #2

    b      12b

13: /* We need scheduling again! */
    /* Check whether the interrupt task thread needs to be set runnable. */
    ldrb   w3, [x20, #1]
    cbz    w3, 4b

    /* It does, so do so. We're using the idle thread stack so no register state preserve needed. */
    bl     _ZN3ams4kern10KScheduler29InterruptTaskThreadToRunnableEv

    /* Clear the interrupt task thread as runnable. */
    strb   wzr, [x20, #1]

    /* Retry the scheduling loop. */
    b      4b
