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
#include <stratosphere.hpp>
#include "dmnt2_hardware_breakpoint.hpp"
#include "dmnt2_debug_process.hpp"
#include "dmnt2_debug_log.hpp"

namespace ams::dmnt {

    namespace {

        constinit auto g_last_bp_register      = -1;
        constinit auto g_first_bp_ctx_register = -1;
        constinit auto g_last_bp_ctx_register  = -1;
        constinit auto g_last_wp_register      = -1;

        constinit os::SdkMutex g_multicore_lock;
        constinit bool g_multicore_started = false;

        alignas(os::ThreadStackAlignment) constinit u8 g_multicore_thread_stack[16_KB];
        constinit os::ThreadType g_multicore_thread;
        constinit os::MessageQueueType g_multicore_request_queue;
        constinit os::MessageQueueType g_multicore_response_queue;
        constinit uintptr_t g_multicore_request_storage[2];
        constinit uintptr_t g_multicore_response_storage[2];

        void MultiCoreThread(void *) {
            /* Get thread core mask. */
            s32 cur_core;
            u64 core_mask;
            R_ABORT_UNLESS(svc::GetThreadCoreMask(std::addressof(cur_core), std::addressof(core_mask), svc::PseudoHandle::CurrentThread));

            /* Service requests. */
            while (true) {
                /* Wait for a request to come in. */
                uintptr_t request_v;
                os::ReceiveMessageQueue(std::addressof(request_v), std::addressof(g_multicore_request_queue));

                /* Process the request. */
                const uintptr_t *request = reinterpret_cast<const uintptr_t *>(request_v);
                bool success = true;
                if (request[0] == 1) {
                    /* Set on each core. */
                    for (s32 core = 0; core < 4; ++core) {
                        /* Switch to the desired core. */
                        R_ABORT_UNLESS(svc::SetThreadCoreMask(svc::PseudoHandle::CurrentThread, core, (1 << core)));

                        /* Get the core mask. */
                        R_ABORT_UNLESS(svc::GetThreadCoreMask(std::addressof(cur_core), std::addressof(core_mask), svc::PseudoHandle::CurrentThread));

                        /* Set the breakpoint. */
                        const Result result = svc::SetHardwareBreakPoint(static_cast<svc::HardwareBreakPointRegisterName>(request[1]), request[2], request[3]);
                        if (R_FAILED(result)) {
                            success = false;
                            AMS_DMNT2_GDB_LOG_ERROR("SetHardwareBreakPoint FAIL 0x%08x, core=%d, reg=%lu, ctrl=%lx, val=%lx\n", result.GetValue(), core, request[1], request[2], request[3]);
                            break;
                        }
                    }
                }

                os::SendMessageQueue(std::addressof(g_multicore_response_queue), static_cast<uintptr_t>(success));
            }
        }

        void EnsureMultiCoreStarted() {
            std::scoped_lock lk(g_multicore_lock);
            if (!g_multicore_started) {
                os::InitializeMessageQueue(std::addressof(g_multicore_request_queue), g_multicore_request_storage, util::size(g_multicore_request_storage));
                os::InitializeMessageQueue(std::addressof(g_multicore_response_queue), g_multicore_response_storage, util::size(g_multicore_response_storage));

                R_ABORT_UNLESS(os::CreateThread(std::addressof(g_multicore_thread), MultiCoreThread, nullptr, g_multicore_thread_stack, sizeof(g_multicore_thread_stack), os::HighestThreadPriority - 1));
                os::StartThread(std::addressof(g_multicore_thread));

                g_multicore_started = true;
            }
        }

    }

    Result HardwareBreakPoint::Clear(DebugProcess *debug_process) {
        AMS_UNUSED(debug_process);

        Result result = svc::ResultInvalidArgument();
        if (m_in_use) {
            AMS_DMNT2_GDB_LOG_DEBUG("HardwareBreakPoint::Clear %p 0x%lx\n", this, m_address);
            result = HardwareBreakPointManager::SetExecutionBreakPoint(m_reg, m_ctx, 0);
            this->Reset();
        }
        return result;
    }

    Result HardwareBreakPoint::Set(DebugProcess *debug_process, uintptr_t address, size_t size, bool is_step) {
        /* Set fields. */
        m_is_step = is_step;
        m_address = address;
        m_size    = size;

        /* Set context breakpoint. */
        R_TRY(HardwareBreakPointManager::SetContextBreakPoint(m_ctx, debug_process));

        /* Set execution breakpoint. */
        R_TRY(HardwareBreakPointManager::SetExecutionBreakPoint(m_reg, m_ctx, address));

        /* Set as in-use. */
        m_in_use = true;
        return ResultSuccess();
    }

    HardwareBreakPointManager::HardwareBreakPointManager(DebugProcess *debug_process) : BreakPointManager(debug_process) {
        /* Determine the number of breakpoint registers. */
        CountBreakPointRegisters();

        /* Initialize all breakpoints. */
        for (size_t i = 0; i < util::size(m_breakpoints); ++i) {
            m_breakpoints[i].Initialize(static_cast<svc::HardwareBreakPointRegisterName>(svc::HardwareBreakPointRegisterName_I0 + i), static_cast<svc::HardwareBreakPointRegisterName>(g_first_bp_ctx_register));
        }
    }

    BreakPointBase *HardwareBreakPointManager::GetBreakPoint(size_t index) {
        if (index < util::size(m_breakpoints)) {
            return m_breakpoints + index;
        } else {
            return nullptr;
        }
    }

    Result HardwareBreakPointManager::SetHardwareBreakPoint(u32 r, u64 dbgbcr, u64 value) {
        /* Send request. */
        const uintptr_t request[4] = {
            1,
            r,
            dbgbcr,
            value
        };
        R_UNLESS(SendMultiCoreRequest(request), dmnt::ResultUnknown());

        return ResultSuccess();
    }

    Result HardwareBreakPointManager::SetContextBreakPoint(svc::HardwareBreakPointRegisterName ctx, DebugProcess *debug_process) {
        /* Encode the register. */
        const u64 dbgbcr = (0x3 << 20) | (0 << 16) | (0xF << 5) | 1;

        const Result result = SetHardwareBreakPoint(ctx, dbgbcr, debug_process->GetHandle());
        if (R_FAILED(result)) {
            AMS_DMNT2_GDB_LOG_ERROR("SetContextBreakPoint FAIL 0x%08x ctx=%d\n", result.GetValue(), ctx);
        }

        return result;
    }

    svc::HardwareBreakPointRegisterName HardwareBreakPointManager::GetWatchPointContextRegister() {
        CountBreakPointRegisters();
        return static_cast<svc::HardwareBreakPointRegisterName>(g_first_bp_ctx_register + 1);
    }

    Result HardwareBreakPointManager::SetExecutionBreakPoint(svc::HardwareBreakPointRegisterName reg, svc::HardwareBreakPointRegisterName ctx, u64 address) {
        /* Encode the register. */
        const u64 dbgbcr = (0x1 << 20) | (ctx << 16) | (0xF << 5) | ((address != 0) ? 1 : 0);

        const Result result = SetHardwareBreakPoint(reg, dbgbcr, address);
        if (R_FAILED(result)) {
            AMS_DMNT2_GDB_LOG_ERROR("SetContextBreakPoint FAIL 0x%08x reg=%d, ctx=%d, address=%lx\n", result.GetValue(), reg, ctx, address);
        }

        return result;
    }

    void HardwareBreakPointManager::CountBreakPointRegisters() {
        /* Determine the valid breakpoint extents. */
        if (g_last_bp_ctx_register == -1) {
            /* Keep setting until we see a failure. */
            for (int i = svc::HardwareBreakPointRegisterName_I0; i <= static_cast<int>(svc::HardwareBreakPointRegisterName_I15); ++i) {
                if (R_FAILED(svc::SetHardwareBreakPoint(static_cast<svc::HardwareBreakPointRegisterName>(i), 0, 0))) {
                    break;
                }
                g_last_bp_register = i;
            }
            AMS_DMNT2_GDB_LOG_DEBUG("Last valid breakpoint=%d\n", g_last_bp_register);

            /* Determine the context register range. */
            const u64 dbgbcr = (0x3 << 20) | (0x0 << 16) | (0xF << 5) | 1;

            g_last_bp_ctx_register = g_last_bp_register;
            for (int i = g_last_bp_ctx_register; i >= static_cast<int>(svc::HardwareBreakPointRegisterName_I0); --i) {
                const Result result = svc::SetHardwareBreakPoint(static_cast<svc::HardwareBreakPointRegisterName>(i), dbgbcr, svc::PseudoHandle::CurrentProcess);
                svc::SetHardwareBreakPoint(static_cast<svc::HardwareBreakPointRegisterName>(i), 0, 0);

                if (R_FAILED(result)) {
                    if (!svc::ResultInvalidHandle::Includes(result)) {
                        break;
                    }
                }

                g_first_bp_ctx_register = i;
            }
            AMS_DMNT2_GDB_LOG_DEBUG("Context BreakPoints = %d-%d\n", g_first_bp_ctx_register, g_last_bp_ctx_register);

            /* Determine valid watchpoint registers. */
            for (int i = svc::HardwareBreakPointRegisterName_D0; i <= static_cast<int>(svc::HardwareBreakPointRegisterName_D15); ++i) {
                if (R_FAILED(svc::SetHardwareBreakPoint(static_cast<svc::HardwareBreakPointRegisterName>(i), 0, 0))) {
                    break;
                }
                g_last_wp_register = i - svc::HardwareBreakPointRegisterName_D0;
            }
            AMS_DMNT2_GDB_LOG_DEBUG("Last valid watchpoint=%d\n", g_last_wp_register);
        }
    }

    bool HardwareBreakPointManager::SendMultiCoreRequest(const void *request) {
        /* Ensure the multi core thread is active. */
        EnsureMultiCoreStarted();

        /* Send the request. */
        os::SendMessageQueue(std::addressof(g_multicore_request_queue), reinterpret_cast<uintptr_t>(request));

        /* Get the response. */
        uintptr_t response;
        os::ReceiveMessageQueue(std::addressof(response), std::addressof(g_multicore_response_queue));

        return static_cast<bool>(response);
    }

}
