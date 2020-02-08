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
#pragma once
#include <mesosphere/kern_select_cpu.hpp>

namespace ams::kern {

    class KThread;
    class KProcess;
    class KScheduler;
    class KInterruptTaskManager;

    struct KCurrentContext {
        std::atomic<KThread *> current_thread;
        std::atomic<KProcess *> current_process;
        KScheduler *scheduler;
        KInterruptTaskManager *interrupt_task_manager;
        s32 core_id;
        void *exception_stack_top;
    };
    static_assert(std::is_pod<KCurrentContext>::value);
    static_assert(sizeof(KCurrentContext) <= cpu::DataCacheLineSize);

    namespace impl {

        ALWAYS_INLINE KCurrentContext &GetCurrentContext() {
            return *reinterpret_cast<KCurrentContext *>(cpu::GetCoreLocalRegionAddress());
        }

    }

    ALWAYS_INLINE KThread *GetCurrentThreadPointer() {
        return impl::GetCurrentContext().current_thread.load(std::memory_order_relaxed);
    }

    ALWAYS_INLINE KThread &GetCurrentThread() {
        return *GetCurrentThreadPointer();
    }

    ALWAYS_INLINE KProcess *GetCurrentProcessPointer() {
        return impl::GetCurrentContext().current_process.load(std::memory_order_relaxed);
    }

    ALWAYS_INLINE KProcess &GetCurrentProcess() {
        return *GetCurrentProcessPointer();
    }

    ALWAYS_INLINE KScheduler *GetCurrentSchedulerPointer() {
        return impl::GetCurrentContext().scheduler;
    }

    ALWAYS_INLINE KScheduler &GetCurrentScheduler() {
        return *GetCurrentSchedulerPointer();
    }

    ALWAYS_INLINE KInterruptTaskManager *GetCurrentInterruptTaskManagerPointer() {
        return impl::GetCurrentContext().interrupt_task_manager;
    }

    ALWAYS_INLINE KInterruptTaskManager &GetCurrentInterruptTaskManager() {
        return *GetCurrentInterruptTaskManagerPointer();
    }

    ALWAYS_INLINE s32 GetCurrentCoreId() {
        return impl::GetCurrentContext().core_id;
    }

    ALWAYS_INLINE void SetCurrentThread(KThread *new_thread) {
        impl::GetCurrentContext().current_thread = new_thread;
    }

    ALWAYS_INLINE void SetCurrentProcess(KProcess *new_process) {
        impl::GetCurrentContext().current_process = new_process;
    }

}
