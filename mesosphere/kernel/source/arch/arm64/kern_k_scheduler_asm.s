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
#include <mesosphere/kern_select_assembly_macros.h>

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
    sub    x2, x2, #(THREAD_STACK_PARAMETERS_SIZE)

    /* Get a reference to the current thread's context. */
    add    x3, x2, #(THREAD_STACK_PARAMETERS_THREAD_CONTEXT)

    /* Save the callee-save registers + SP. */
    stp    x19,  x20, [x3, #(THREAD_CONTEXT_X19_X20)]
    stp    x21,  x22, [x3, #(THREAD_CONTEXT_X21_X22)]
    stp    x23,  x24, [x3, #(THREAD_CONTEXT_X23_X24)]
    stp    x25,  x26, [x3, #(THREAD_CONTEXT_X25_X26)]
    stp    x27,  x28, [x3, #(THREAD_CONTEXT_X27_X28)]
    stp    x29,  x30, [x3, #(THREAD_CONTEXT_X29_X30)]

    mov    x4, sp
    str    x4, [x3, #(THREAD_CONTEXT_SP)]

    /* Check if the fpu is enabled; if it is, we need to save it. */
    mrs    x5, cpacr_el1
    and    x4, x5, #0x300000
    cbz    x4, 8f

    /* We need to save the fpu state; save fpsr/fpcr. */
    mrs    x4, fpcr
    mrs    x6, fpsr
    stp    w4, w6, [x3, #(THREAD_CONTEXT_FPCR_FPSR)]

    /* Set fpu-restore-needed in our exception flags. */
    ldrb   w4, [x2, #(THREAD_STACK_PARAMETERS_EXCEPTION_FLAGS)]
    orr    w4, w4, #(THREAD_EXCEPTION_FLAG_IS_FPU_CONTEXT_RESTORE_NEEDED)
    strb   w4, [x2, #(THREAD_STACK_PARAMETERS_EXCEPTION_FLAGS)]

    /* We need to save fpu state based on whether we're a 64-bit or 32-bit thread. */
    tbz    w4, #(THREAD_EXCEPTION_FLAG_BIT_INDEX_IS_FPU_64_BIT), 4f

    /* We have a 64-bit fpu. */

    /* If we're in a usermode exception, we need to save the caller-save fpu registers. */
    tbz    w4, #(THREAD_EXCEPTION_FLAG_BIT_INDEX_IS_IN_USERMODE_EXCEPTION_HANDLER), 2f

    /* If we're in an SVC (and not a usermode exception), we only need to save the callee-save fpu registers. */
    tbz    w4, #(THREAD_EXCEPTION_FLAG_BIT_INDEX_IS_CALLING_SVC), 3f

2:  /* Save the 64-bit caller-save fpu registers. */
    ldr    x6, [x2, #(THREAD_STACK_PARAMETERS_CALLER_SAVE_FPU_REGISTERS)]
    stp    q0,  q1,  [x6, #(THREAD_FPU64_CONTEXT_Q0_Q1)]
    stp    q2,  q3,  [x6, #(THREAD_FPU64_CONTEXT_Q2_Q3)]
    stp    q4,  q5,  [x6, #(THREAD_FPU64_CONTEXT_Q4_Q5)]
    stp    q6,  q7,  [x6, #(THREAD_FPU64_CONTEXT_Q6_Q7)]
    stp    q16, q17, [x6, #(THREAD_FPU64_CONTEXT_Q16_Q17)]
    stp    q18, q19, [x6, #(THREAD_FPU64_CONTEXT_Q18_Q19)]
    stp    q20, q21, [x6, #(THREAD_FPU64_CONTEXT_Q20_Q21)]
    stp    q22, q23, [x6, #(THREAD_FPU64_CONTEXT_Q22_Q23)]
    stp    q24, q25, [x6, #(THREAD_FPU64_CONTEXT_Q24_Q25)]
    stp    q26, q27, [x6, #(THREAD_FPU64_CONTEXT_Q26_Q27)]
    stp    q28, q29, [x6, #(THREAD_FPU64_CONTEXT_Q28_Q29)]
    stp    q30, q31, [x6, #(THREAD_FPU64_CONTEXT_Q30_Q31)]

3:  /* Save the 64-bit callee-save fpu registers. */
    stp    q8,  q9,  [x3, #(THREAD_CONTEXT_FPU64_Q8_Q9)]
    stp    q10, q11, [x3, #(THREAD_CONTEXT_FPU64_Q10_Q11)]
    stp    q12, q13, [x3, #(THREAD_CONTEXT_FPU64_Q12_Q13)]
    stp    q14, q15, [x3, #(THREAD_CONTEXT_FPU64_Q14_Q15)]
    b      7f

4:  /* We have a 32-bit fpu. */

    /* If we're in a usermode exception, we need to save the caller-save fpu registers. */
    tbz    w4, #(THREAD_EXCEPTION_FLAG_BIT_INDEX_IS_IN_USERMODE_EXCEPTION_HANDLER), 5f

    /* If we're in an SVC (and not a usermode exception), we only need to save the callee-save fpu registers. */
    tbz    w4, #(THREAD_EXCEPTION_FLAG_BIT_INDEX_IS_CALLING_SVC), 6f

5:  /* Save the 32-bit caller-save fpu registers. */
    ldr    x6, [x2, #(THREAD_STACK_PARAMETERS_CALLER_SAVE_FPU_REGISTERS)]
    stp    q0,  q1,  [x6, #(THREAD_FPU32_CONTEXT_Q0_Q1)]
    stp    q2,  q3,  [x6, #(THREAD_FPU32_CONTEXT_Q2_Q3)]
    stp    q8,  q9,  [x6, #(THREAD_FPU32_CONTEXT_Q8_Q9)]
    stp    q10, q11, [x6, #(THREAD_FPU32_CONTEXT_Q10_Q11)]
    stp    q12, q13, [x6, #(THREAD_FPU32_CONTEXT_Q12_Q13)]
    stp    q14, q15, [x6, #(THREAD_FPU32_CONTEXT_Q14_Q15)]

6:  /* Save the 32-bit callee-save fpu registers. */
    stp    q4,  q5,  [x3, #(THREAD_CONTEXT_FPU32_Q4_Q5)]
    stp    q6,  q7,  [x3, #(THREAD_CONTEXT_FPU32_Q6_Q7)]

7:  /* With the fpu state saved, disable the fpu. */
    and    x5, x5, #(~0x300000)
    msr    cpacr_el1, x5

8:  /* We're done saving this thread's context. */

    /* Check if the thread is terminated by checking the DPC flags for DpcFlag_Terminated. */
    ldrb   w4, [x2, #(THREAD_STACK_PARAMETERS_DPC_FLAGS)]
    tbnz   w4, #1, 9f

    /* The thread isn't terminated, so we want to unlock it. */
    /* Write atomically to the context's locked member. */
    add    x3, x3, #(THREAD_CONTEXT_LOCKED)
    stlrb  wzr, [x3]

9:  /* The current thread's context has been entirely taken care of. */
    /* Now we want to loop until we successfully switch the thread context. */
    /* Start by saving all the values we care about in callee-save registers. */
    mov    x19, x0 /* this */
    mov    x20, x1 /* this->state */
    mov    x21, x7 /* highest priority thread */

    /* Set our stack to the idle thread stack. */
    ldr    x3, [x20, #(KSCHEDULER_IDLE_THREAD_STACK)]
    mov    sp, x3
    b      11f

10: /* We failed to successfully do the context switch, and need to retry. */
    /* Clear the exclusive monitor. */
    clrex

    /* Clear the need's scheduling bool (and dmb ish after, as it's an atomic). */
    strb   wzr, [x20]
    dmb    ish

    /* Refresh the highest priority thread. */
    ldr    x21, [x20, #(KSCHEDULER_HIGHEST_PRIORITY_THREAD)]

11: /* We're starting to try to do the context switch. */
    /* Check if the highest priority thread if null. */
    /* If it is, we want to branch to a special idle thread loop. */
    cbz    x21, 16f

    /* Get the highest priority thread's context, and save it. */
    /* ams::kern::KThread::GetContextForSchedulerLoop() */
    ldr    x22, [x21, #(THREAD_KERNEL_STACK_TOP)]
    sub    x22, x22, #(THREAD_STACK_PARAMETERS_SIZE - THREAD_STACK_PARAMETERS_THREAD_CONTEXT)

    /* Prepare to try to acquire the context lock. */
    add    x1, x22, #(THREAD_CONTEXT_LOCKED)
    mov    w2, #1

12: /* We want to try to lock the highest priority thread's context. */
    /* Check if the lock is already held. */
    ldaxrb w3, [x1]
    cbnz   w3, 13f

    /* If it's not, try to take it. */
    stxrb  w3, w2, [x1]
    cbnz   w3, 12b

    /* We hold the lock, so we can now switch the thread. */
    b      14f

13: /* The highest priority thread's context is already locked. */
    /* Check if we need scheduling. If we don't, we can retry directly. */
    ldarb  w3, [x20] // ldarb w3, [x20, #(KSCHEDULER_NEEDS_SCHEDULING)]
    cbz    w3, 12b

    /* If we do, another core is interfering, and we must start from the top. */
    b      10b

14: /* It's time to switch the thread. */
    /* Switch to the highest priority thread. */
    mov    x0, x19
    mov    x1, x21

    /* Call ams::kern::KScheduler::SwitchThread(ams::kern::KThread *) */
    bl     _ZN3ams4kern10KScheduler12SwitchThreadEPNS0_7KThreadE

    /* Check if we need scheduling. If we do, then we can't complete the switch and should retry. */
    ldarb  w1, [x20] // ldarb w1, [x20, #(KSCHEDULER_NEEDS_SCHEDULING)]
    cbnz   w1, 15f

    /* Restore the thread context. */
    mov    x0, x22
    ldp    x19, x20, [x0, #(THREAD_CONTEXT_X19_X20)]
    ldp    x21, x22, [x0, #(THREAD_CONTEXT_X21_X22)]
    ldp    x23, x24, [x0, #(THREAD_CONTEXT_X23_X24)]
    ldp    x25, x26, [x0, #(THREAD_CONTEXT_X25_X26)]
    ldp    x27, x28, [x0, #(THREAD_CONTEXT_X27_X28)]
    ldp    x29, x30, [x0, #(THREAD_CONTEXT_X29_X30)]

    ldr    x1, [x0, #(THREAD_CONTEXT_SP)]
    mov    sp, x1

    /* Return. */
    ret

15: /* Our switch failed. */
    /* We should unlock the thread context, and then retry. */
    add    x1, x22, #(THREAD_CONTEXT_LOCKED)
    stlrb  wzr, [x1]
    b      10b

16: /* The next thread is nullptr! */
    /* Switch to nullptr. This will actually switch to the idle thread. */
    mov    x0, x19
    mov    x1, #0

    /* Call ams::kern::KScheduler::SwitchThread(ams::kern::KThread *) */
    bl     _ZN3ams4kern10KScheduler12SwitchThreadEPNS0_7KThreadE

17: /* We've switched to the idle thread, so we want to process interrupt tasks until we schedule a non-idle thread. */
    /* Check whether there are runnable interrupt tasks. */
    ldrb   w3, [x20, #(KSCHEDULER_INTERRUPT_TASK_RUNNABLE)]
    cbnz   w3, 18f

    /* Check if we need scheduling. */
    ldarb  w3, [x20] // ldarb w3, [x20, #(KSCHEDULER_NEEDS_SCHEDULING)]
    cbnz   w3, 10b

    /* Clear the previous thread. */
    str    xzr, [x20, #(KSCHEDULER_PREVIOUS_THREAD)]

    /* Wait for an interrupt and check again. */
    wfi

    msr    daifclr, #2
    msr    daifset, #2

    b      17b

18: /* We have interrupt tasks to execute! */
    /* Execute any pending interrupt tasks. */
    ldr    x0, [x20, #(KSCHEDULER_INTERRUPT_TASK_MANAGER)]
    bl     _ZN3ams4kern21KInterruptTaskManager7DoTasksEv

    /* Clear the interrupt task thread as runnable. */
    strb   wzr, [x20, #(KSCHEDULER_INTERRUPT_TASK_RUNNABLE)]

    /* Retry the scheduling loop. */
    b      10b
