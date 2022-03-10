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

namespace ams::diag::impl {

    namespace {

        uintptr_t GetAddressValue(uintptr_t address) {
            if (address == 0) {
                return 0;
            }

            if (!util::IsAligned(address, alignof(uintptr_t))) {
                return 0;
            }

            return *reinterpret_cast<uintptr_t *>(address);
        }

        template<typename T, size_t N>
        svc::MemoryInfo *GetMemoryInfoPointer(T (&arr)[N]) {
            /* Check that there's enough space. */
            static_assert(sizeof(T) * N <= sizeof(svc::MemoryInfo));
            static_assert(alignof(T) >= alignof(svc::MemoryInfo));
            return reinterpret_cast<svc::MemoryInfo *>(std::addressof(arr[0]));
        }

        bool IsValidLinkRegisterValue(uintptr_t lr, svc::MemoryInfo *info) {
            /* Ensure the memory info is valid. */
            Result query_res;

            if (!(info->base_address <= lr && lr < info->base_address + info->size) && ({ svc::PageInfo page_info; query_res = svc::QueryMemory(info, std::addressof(page_info), lr); R_FAILED(query_res); })) {
                AMS_SDK_LOG("Failed to get backtrace. Query memory failed. (lr: %p, result: %03d-%04d)\n", reinterpret_cast<void *>(lr), query_res.GetModule(), query_res.GetDescription());
                return false;
            }

            /* Check that lr is valid. */
            if (lr == 0) {
                return false;
            }

            if (!util::IsAligned(lr, sizeof(u32))) {
                AMS_SDK_LOG("Failed to get backtrace. The link register alignment is invalid. (lr: %p)\n", reinterpret_cast<void *>(lr));
                return false;
            }

            /* Check that the lr points to code. */
            if (info->permission != svc::MemoryPermission_ReadExecute) {
                AMS_SDK_LOG("Failed to get backtrace. The link register points out of the code. (lr: %p)\n", reinterpret_cast<void *>(lr));
                return false;
            }

            return true;
        }

        void GetNormalStackInfo(Backtrace::StackInfo *out) {
            if (void * const fiber = nullptr /* TODO: os::GetCurrentFiber() */; fiber == nullptr) {
                /* Get thread. */
                auto * const thread = os::GetCurrentThread();
                out->stack_top    = reinterpret_cast<uintptr_t>(thread->stack);
                out->stack_bottom = reinterpret_cast<uintptr_t>(thread->stack) + thread->stack_size;
            } else {
                /* TODO: Fiber. */
            }
        }

        bool GetExceptionStackInfo(Backtrace::StackInfo *out, uintptr_t sp) {
            /* Get the current stack info. */
            uintptr_t cur_stack   = 0;
            size_t cur_stack_size = 0;
            os::GetCurrentStackInfo(std::addressof(cur_stack), std::addressof(cur_stack_size));

            /* Get the thread's stack info. */
            uintptr_t thread_stack   = 0;
            size_t thread_stack_size = 0;
            os::GetThreadStackInfo(std::addressof(thread_stack), std::addressof(thread_stack_size), os::GetCurrentThread());

            /* If the current stack is the thread stack, exception stack isn't being used. */
            if (cur_stack == thread_stack) {
                AMS_ASSERT(cur_stack_size == thread_stack_size);
                return false;
            }

            /* Check if the stack pointer is contained in the current stack. */
            if (!(cur_stack <= sp && sp < cur_stack + cur_stack_size)) {
                return false;
            }

            /* Set the output. */
            out->stack_top    = cur_stack;
            out->stack_bottom = cur_stack + cur_stack_size;
            return true;
        }

    }


    NOINLINE void Backtrace::Initialize() {
        /* Get the stack pointer/frame pointer. */
        uintptr_t fp, sp;

        __asm__ __volatile__(
            #if defined(ATMOSPHERE_ARCH_ARM64)
                "mov %[fp], fp\n"
                "mov %[sp], sp\n"
            #elif defined(ATMOSPHERE_ARCH_ARM)
                "mov %[fp], x29\n"
                "mov %[sp], sp\n"
            #else
                #error "Unknown architecture for Horizon fp/sp retrieval."
            #endif
            : [fp]"=&r"(fp), [sp]"=&r"(sp)
            :
            : "memory"
        );

        /* Set our stack info. */
        this->SetStackInfo(fp, sp);

        /* Step, to get our first lr. */
        this->Step();
    }

    void Backtrace::Initialize(uintptr_t fp, uintptr_t sp, uintptr_t pc) {
        /* Set our initial lr. */
        m_lr = pc;

        /* Set our stack info. */
        this->SetStackInfo(fp, sp);
    }

    bool Backtrace::Step() {
        /* We can't step without a frame pointer. */
        if (m_fp == 0) {
            if (m_current_stack_info != std::addressof(m_normal_stack_info)) {
                AMS_SDK_LOG("Failed to get backtrace. The frame pointer is null.\n");
            }
            return false;
        }

        /* The frame pointer needs to be aligned. */
        if (!util::IsAligned(m_fp, sizeof(uintptr_t))) {
            AMS_SDK_LOG("Failed to get backtrace. The frame pointer alignment is invalid. (fp: %p)\n", reinterpret_cast<void *>(m_fp));
            return false;
        }

        /* Ensure our current stack info is good. */
        if (!(m_current_stack_info->stack_top <= m_fp && m_fp < m_current_stack_info->stack_bottom)) {
            if (m_current_stack_info != std::addressof(m_exception_stack_info) || !(m_normal_stack_info.stack_top <= m_fp && m_fp < m_normal_stack_info.stack_bottom)) {
                AMS_SDK_LOG("Failed to get backtrace. The frame pointer points out of the stack. (fp: %p, stack: %p-%p)\n", reinterpret_cast<void *>(m_fp), reinterpret_cast<void *>(m_current_stack_info->stack_top), reinterpret_cast<void *>(m_current_stack_info->stack_bottom));
                return false;
            }

            m_current_stack_info = std::addressof(m_normal_stack_info);
        } else if (m_fp <= m_prev_fp) {
            AMS_SDK_LOG("Failed to get backtrace. The frame pointer is rewinding. (fp: %p, prev fp: %p, stack: %p-%p)\n", reinterpret_cast<void *>(m_fp), reinterpret_cast<void *>(m_prev_fp), reinterpret_cast<void *>(m_current_stack_info->stack_top), reinterpret_cast<void *>(m_current_stack_info->stack_bottom));
            return false;
        }

        /* Update our previous fp. */
        m_prev_fp = m_fp;

        /* Read lr/fp. */
        m_lr = GetAddressValue(m_fp + sizeof(m_fp));
        m_fp = GetAddressValue(m_fp);

        /* Check that lr is valid. */
        if (IsValidLinkRegisterValue(m_lr, GetMemoryInfoPointer(m_memory_info_buffer))) {
            return true;
        } else {
            m_lr = 0;
            return false;
        }
    }

    uintptr_t Backtrace::GetStackPointer() const {
        if (m_fp != 0) {
            return m_fp - sizeof(m_fp);
        } else {
            return m_current_stack_info->stack_bottom - sizeof(m_fp);
        }
    }

    uintptr_t Backtrace::GetReturnAddress() const {
        return m_lr;
    }

    void Backtrace::SetStackInfo(uintptr_t fp, uintptr_t sp) {
        /* Get the normal stack info. */
        GetNormalStackInfo(std::addressof(m_normal_stack_info));

        /* Get the exception stack info. */
        if (GetExceptionStackInfo(std::addressof(m_exception_stack_info), sp)) {
            m_current_stack_info = std::addressof(m_exception_stack_info);
        } else {
            m_current_stack_info = std::addressof(m_normal_stack_info);
        }

        /* Set our frame pointer. */
        m_fp = fp;
    }

}
