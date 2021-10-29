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
#pragma once
#include <mesosphere/kern_build_config.hpp>

/* TODO: Different header for this? */
#define AMS_KERN_NUM_SUPERVISOR_CALLS 0xC0

/* ams::kern::KThread, https://github.com/Atmosphere-NX/Atmosphere/blob/master/libraries/libmesosphere/include/mesosphere/kern_k_thread.hpp */
#define THREAD_THREAD_CONTEXT 0xD0

/* ams::kern::KThread::StackParameters, https://github.com/Atmosphere-NX/Atmosphere/blob/master/libraries/libmesosphere/include/mesosphere/kern_k_thread.hpp */
#define THREAD_STACK_PARAMETERS_SIZE                    0x30
#define THREAD_STACK_PARAMETERS_SVC_PERMISSION          0x00
#define THREAD_STACK_PARAMETERS_CONTEXT                 0x18
#define THREAD_STACK_PARAMETERS_CUR_THREAD              0x20
#define THREAD_STACK_PARAMETERS_DISABLE_COUNT           0x28
#define THREAD_STACK_PARAMETERS_DPC_FLAGS               0x2A
#define THREAD_STACK_PARAMETERS_CURRENT_SVC_ID          0x2B
#define THREAD_STACK_PARAMETERS_IS_CALLING_SVC          0x2C
#define THREAD_STACK_PARAMETERS_IS_IN_EXCEPTION_HANDLER 0x2D
#define THREAD_STACK_PARAMETERS_IS_PINNED               0x2E

#if defined(MESOSPHERE_ENABLE_HARDWARE_SINGLE_STEP)
#define THREAD_STACK_PARAMETERS_IS_SINGLE_STEP          0x2F
#endif

/* ams::kern::arch::arm64::KThreadContext, https://github.com/Atmosphere-NX/Atmosphere/blob/master/libraries/libmesosphere/include/mesosphere/arch/arm64/kern_k_thread_context.hpp */
#define THREAD_CONTEXT_SIZE          0x290
#define THREAD_CONTEXT_CPU_REGISTERS 0x000
#define THREAD_CONTEXT_X19           0x000
#define THREAD_CONTEXT_X20           0x008
#define THREAD_CONTEXT_X21           0x010
#define THREAD_CONTEXT_X22           0x018
#define THREAD_CONTEXT_X23           0x020
#define THREAD_CONTEXT_X24           0x028
#define THREAD_CONTEXT_X25           0x030
#define THREAD_CONTEXT_X26           0x038
#define THREAD_CONTEXT_X27           0x040
#define THREAD_CONTEXT_X28           0x048
#define THREAD_CONTEXT_X29           0x050
#define THREAD_CONTEXT_LR            0x058
#define THREAD_CONTEXT_SP            0x060
#define THREAD_CONTEXT_CPACR         0x068
#define THREAD_CONTEXT_FPCR          0x070
#define THREAD_CONTEXT_FPSR          0x078
#define THREAD_CONTEXT_FPU_REGISTERS 0x080
#define THREAD_CONTEXT_LOCKED        0x280

#define THREAD_CONTEXT_X19_X20   THREAD_CONTEXT_X19
#define THREAD_CONTEXT_X21_X22   THREAD_CONTEXT_X21
#define THREAD_CONTEXT_X23_X24   THREAD_CONTEXT_X23
#define THREAD_CONTEXT_X25_X26   THREAD_CONTEXT_X25
#define THREAD_CONTEXT_X27_X28   THREAD_CONTEXT_X27
#define THREAD_CONTEXT_X29_X30   THREAD_CONTEXT_X29
#define THREAD_CONTEXT_LR_SP     THREAD_CONTEXT_LR
#define THREAD_CONTEXT_SP_CPACR  THREAD_CONTEXT_SP
#define THREAD_CONTEXT_FPCR_FPSR THREAD_CONTEXT_FPCR

/* ams::kern::arch::arm64::KExceptionContext, https://github.com/Atmosphere-NX/Atmosphere/blob/master/libraries/libmesosphere/include/mesosphere/arch/arm64/kern_k_exception_context.hpp */
#define EXCEPTION_CONTEXT_SIZE  0x120
#define EXCEPTION_CONTEXT_X0    0x000
#define EXCEPTION_CONTEXT_X1    0x008
#define EXCEPTION_CONTEXT_X2    0x010
#define EXCEPTION_CONTEXT_X3    0x018
#define EXCEPTION_CONTEXT_X4    0x020
#define EXCEPTION_CONTEXT_X5    0x028
#define EXCEPTION_CONTEXT_X6    0x030
#define EXCEPTION_CONTEXT_X7    0x038
#define EXCEPTION_CONTEXT_X8    0x040
#define EXCEPTION_CONTEXT_X9    0x048
#define EXCEPTION_CONTEXT_X10   0x050
#define EXCEPTION_CONTEXT_X11   0x058
#define EXCEPTION_CONTEXT_X12   0x060
#define EXCEPTION_CONTEXT_X13   0x068
#define EXCEPTION_CONTEXT_X14   0x070
#define EXCEPTION_CONTEXT_X15   0x078
#define EXCEPTION_CONTEXT_X16   0x080
#define EXCEPTION_CONTEXT_X17   0x088
#define EXCEPTION_CONTEXT_X18   0x090
#define EXCEPTION_CONTEXT_X19   0x098
#define EXCEPTION_CONTEXT_X20   0x0A0
#define EXCEPTION_CONTEXT_X21   0x0A8
#define EXCEPTION_CONTEXT_X22   0x0B0
#define EXCEPTION_CONTEXT_X23   0x0B8
#define EXCEPTION_CONTEXT_X24   0x0C0
#define EXCEPTION_CONTEXT_X25   0x0C8
#define EXCEPTION_CONTEXT_X26   0x0D0
#define EXCEPTION_CONTEXT_X27   0x0D8
#define EXCEPTION_CONTEXT_X28   0x0E0
#define EXCEPTION_CONTEXT_X29   0x0E8
#define EXCEPTION_CONTEXT_X30   0x0F0
#define EXCEPTION_CONTEXT_SP    0x0F8
#define EXCEPTION_CONTEXT_PC    0x100
#define EXCEPTION_CONTEXT_PSR   0x108
#define EXCEPTION_CONTEXT_TPIDR 0x110

#define EXCEPTION_CONTEXT_X0_X1   EXCEPTION_CONTEXT_X0
#define EXCEPTION_CONTEXT_X2_X3   EXCEPTION_CONTEXT_X2
#define EXCEPTION_CONTEXT_X4_X5   EXCEPTION_CONTEXT_X4
#define EXCEPTION_CONTEXT_X6_X7   EXCEPTION_CONTEXT_X6
#define EXCEPTION_CONTEXT_X8_X9   EXCEPTION_CONTEXT_X8
#define EXCEPTION_CONTEXT_X10_X11 EXCEPTION_CONTEXT_X10
#define EXCEPTION_CONTEXT_X12_X13 EXCEPTION_CONTEXT_X12
#define EXCEPTION_CONTEXT_X14_X15 EXCEPTION_CONTEXT_X14
#define EXCEPTION_CONTEXT_X16_X17 EXCEPTION_CONTEXT_X16
#define EXCEPTION_CONTEXT_X18_X19 EXCEPTION_CONTEXT_X18
#define EXCEPTION_CONTEXT_X20_X21 EXCEPTION_CONTEXT_X20
#define EXCEPTION_CONTEXT_X22_X23 EXCEPTION_CONTEXT_X22
#define EXCEPTION_CONTEXT_X24_X25 EXCEPTION_CONTEXT_X24
#define EXCEPTION_CONTEXT_X26_X27 EXCEPTION_CONTEXT_X26
#define EXCEPTION_CONTEXT_X28_X29 EXCEPTION_CONTEXT_X28
#define EXCEPTION_CONTEXT_X30_SP  EXCEPTION_CONTEXT_X30
#define EXCEPTION_CONTEXT_PC_PSR  EXCEPTION_CONTEXT_PC

#define EXCEPTION_CONTEXT_X9_X10    EXCEPTION_CONTEXT_X9
#define EXCEPTION_CONTEXT_X19_X20   EXCEPTION_CONTEXT_X19
#define EXCEPTION_CONTEXT_X21_X22   EXCEPTION_CONTEXT_X21
#define EXCEPTION_CONTEXT_X23_X24   EXCEPTION_CONTEXT_X23
#define EXCEPTION_CONTEXT_X25_X26   EXCEPTION_CONTEXT_X25
#define EXCEPTION_CONTEXT_X27_X28   EXCEPTION_CONTEXT_X27
#define EXCEPTION_CONTEXT_X29_X30   EXCEPTION_CONTEXT_X29
#define EXCEPTION_CONTEXT_SP_PC     EXCEPTION_CONTEXT_SP
#define EXCEPTION_CONTEXT_PSR_TPIDR EXCEPTION_CONTEXT_PSR

/* ams::svc::arch::arm64::ThreadLocalRegion, https://github.com/Atmosphere-NX/Atmosphere/blob/master/libraries/libvapours/include/vapours/svc/arch/arm64/svc_thread_local_region.hpp */
#define THREAD_LOCAL_REGION_MESSAGE_BUFFER 0x000
#define THREAD_LOCAL_REGION_DISABLE_COUNT  0x100
#define THREAD_LOCAL_REGION_INTERRUPT_FLAG 0x102
#define THREAD_LOCAL_REGION_SIZE           0x200

/* ams::kern::init::KInitArguments, https://github.com/Atmosphere-NX/Atmosphere/blob/master/libraries/libmesosphere/include/mesosphere/arch/arm64/init/kern_k_init_arguments.hpp */
#define INIT_ARGUMENTS_SIZE            0x60
#define INIT_ARGUMENTS_TTBR0           0x00
#define INIT_ARGUMENTS_TTBR1           0x08
#define INIT_ARGUMENTS_TCR             0x10
#define INIT_ARGUMENTS_MAIR            0x18
#define INIT_ARGUMENTS_CPUACTLR        0x20
#define INIT_ARGUMENTS_CPUECTLR        0x28
#define INIT_ARGUMENTS_SCTLR           0x30
#define INIT_ARGUMENTS_SP              0x38
#define INIT_ARGUMENTS_ENTRYPOINT      0x40
#define INIT_ARGUMENTS_ARGUMENT        0x48
#define INIT_ARGUMENTS_SETUP_FUNCTION  0x50
#define INIT_ARGUMENTS_EXCEPTION_STACK 0x58

/* ams::kern::KScheduler (::SchedulingState), https://github.com/Atmosphere-NX/Atmosphere/blob/master/libraries/libmesosphere/include/mesosphere/kern_k_scheduler.hpp */
/* NOTE: Due to constraints on ldarb relative offsets, KSCHEDULER_NEEDS_SCHEDULING cannot trivially be changed, and will require assembly edits. */
#define KSCHEDULER_NEEDS_SCHEDULING        0x00
#define KSCHEDULER_INTERRUPT_TASK_RUNNABLE 0x01
#define KSCHEDULER_HIGHEST_PRIORITY_THREAD 0x10
#define KSCHEDULER_IDLE_THREAD_STACK       0x18
#define KSCHEDULER_PREVIOUS_THREAD         0x20
#define KSCHEDULER_INTERRUPT_TASK_MANAGER  0x28
