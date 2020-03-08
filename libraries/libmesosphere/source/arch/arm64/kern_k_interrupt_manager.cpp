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
#include <mesosphere.hpp>

namespace ams::kern::arch::arm64 {

    /* Instantiate static members in specific translation unit. */
    KSpinLock KInterruptManager::s_lock;
    std::array<KInterruptManager::KGlobalInterruptEntry, KInterruptController::NumGlobalInterrupts> KInterruptManager::s_global_interrupts;
    KInterruptController::GlobalState KInterruptManager::s_global_state;
    bool KInterruptManager::s_global_state_saved;

    void KInterruptManager::Initialize(s32 core_id) {
        this->interrupt_controller.Initialize(core_id);
    }

    void KInterruptManager::Finalize(s32 core_id) {
        this->interrupt_controller.Finalize(core_id);
    }

    bool KInterruptManager::OnHandleInterrupt() {
        /* Get the interrupt id. */
        const u32 raw_irq = this->interrupt_controller.GetIrq();
        const s32 irq = KInterruptController::ConvertRawIrq(raw_irq);

        /* If the IRQ is spurious, we don't need to reschedule. */
        if (irq < 0) {
            return false;
        }

        KInterruptTask *task = nullptr;
        if (KInterruptController::IsLocal(irq)) {
            /* Get local interrupt entry. */
            auto &entry = GetLocalInterruptEntry(irq);
            if (entry.handler != nullptr) {
                /* Set manual clear needed if relevant. */
                if (entry.manually_cleared) {
                    this->interrupt_controller.SetPriorityLevel(irq, KInterruptController::PriorityLevel_Low);
                    entry.needs_clear = true;
                }

                /* Set the handler. */
                task = entry.handler->OnInterrupt(irq);
            } else {
                MESOSPHERE_LOG("Core%d: Unhandled local interrupt %d\n", GetCurrentCoreId(), irq);
            }
        } else if (KInterruptController::IsGlobal(irq)) {
            KScopedSpinLock lk(GetLock());

            /* Get global interrupt entry. */
            auto &entry = GetGlobalInterruptEntry(irq);
            if (entry.handler != nullptr) {
                /* Set manual clear needed if relevant. */
                if (entry.manually_cleared) {
                    this->interrupt_controller.Disable(irq);
                    entry.needs_clear = true;
                }

                /* Set the handler. */
                task = entry.handler->OnInterrupt(irq);
            } else {
                MESOSPHERE_LOG("Core%d: Unhandled global interrupt %d\n", GetCurrentCoreId(), irq);
            }
        } else {
            MESOSPHERE_LOG("Invalid interrupt %d\n", irq);
        }

        /* Acknowledge the interrupt. */
        this->interrupt_controller.EndOfInterrupt(raw_irq);

        /* If we found no task, then we don't need to reschedule. */
        if (task == nullptr) {
            return false;
        }

        /* If the task isn't the dummy task, we should add it to the queue. */
        if (task != GetDummyInterruptTask()) {
            Kernel::GetInterruptTaskManager().EnqueueTask(task);
        }

        return true;
    }

    void KInterruptManager::HandleInterrupt(bool user_mode) {
        /* On interrupt, call OnHandleInterrupt() to determine if we need rescheduling and handle. */
        const bool needs_scheduling = Kernel::GetInterruptManager().OnHandleInterrupt();

        /* If we need scheduling, */
        if (needs_scheduling) {
            /* Handle any changes needed to the user preemption state. */
            if (user_mode && GetCurrentThread().GetUserPreemptionState() != 0 && GetCurrentProcess().GetPreemptionStatePinnedThread(GetCurrentCoreId()) == nullptr) {
                KScopedSchedulerLock sl;

                /* Note the preemption state in process. */
                GetCurrentProcess().SetPreemptionState();

                /* Set the kernel preemption state flag. */
                GetCurrentThread().SetKernelPreemptionState(1);;

                /* Request interrupt scheduling. */
                Kernel::GetScheduler().RequestScheduleOnInterrupt();
            } else {
                /* Request interrupt scheduling. */
                Kernel::GetScheduler().RequestScheduleOnInterrupt();
            }
        }

        /* If user mode, check if the thread needs termination. */
        /* If it does, we can take advantage of this to terminate it. */
        if (user_mode) {
            KThread *cur_thread = GetCurrentThreadPointer();
            if (cur_thread->IsTerminationRequested()) {
                KScopedInterruptEnable ei;
                cur_thread->Exit();
            }
        }
    }

    Result KInterruptManager::BindHandler(KInterruptHandler *handler, s32 irq, s32 core_id, s32 priority, bool manual_clear, bool level) {
        R_UNLESS(KInterruptController::IsGlobal(irq) || KInterruptController::IsLocal(irq), svc::ResultOutOfRange());

        KScopedInterruptDisable di;

        if (KInterruptController::IsGlobal(irq)) {
            KScopedSpinLock lk(GetLock());
            return this->BindGlobal(handler, irq, core_id, priority, manual_clear, level);
        } else {
            MESOSPHERE_ASSERT(core_id == GetCurrentCoreId());
            return this->BindLocal(handler, irq, priority, manual_clear);
        }
    }

    Result KInterruptManager::UnbindHandler(s32 irq, s32 core_id) {
        R_UNLESS(KInterruptController::IsGlobal(irq) || KInterruptController::IsLocal(irq), svc::ResultOutOfRange());

        KScopedInterruptDisable di;

        if (KInterruptController::IsGlobal(irq)) {
            KScopedSpinLock lk(GetLock());
            return this->UnbindGlobal(irq);
        } else {
            MESOSPHERE_ASSERT(core_id == GetCurrentCoreId());
            return this->UnbindLocal(irq);
        }
    }

    Result KInterruptManager::ClearInterrupt(s32 irq) {
        R_UNLESS(KInterruptController::IsGlobal(irq), svc::ResultOutOfRange());

        KScopedInterruptDisable di;
        KScopedSpinLock lk(GetLock());
        return this->ClearGlobal(irq);
    }

    Result KInterruptManager::ClearInterrupt(s32 irq, s32 core_id) {
        R_UNLESS(KInterruptController::IsGlobal(irq) || KInterruptController::IsLocal(irq), svc::ResultOutOfRange());

        KScopedInterruptDisable di;

        if (KInterruptController::IsGlobal(irq)) {
            KScopedSpinLock lk(GetLock());
            return this->ClearGlobal(irq);
        } else {
            MESOSPHERE_ASSERT(core_id == GetCurrentCoreId());
            return this->ClearLocal(irq);
        }
    }

    Result KInterruptManager::BindGlobal(KInterruptHandler *handler, s32 irq, s32 core_id, s32 priority, bool manual_clear, bool level) {
        /* Ensure the priority level is valid. */
        R_UNLESS(KInterruptController::PriorityLevel_High <= priority, svc::ResultOutOfRange());
        R_UNLESS(priority <= KInterruptController::PriorityLevel_Low,  svc::ResultOutOfRange());

        /* Ensure we aren't already bound. */
        auto &entry = GetGlobalInterruptEntry(irq);
        R_UNLESS(entry.handler == nullptr, svc::ResultBusy());

        /* Set entry fields. */
        entry.needs_clear = false;
        entry.manually_cleared = manual_clear;
        entry.handler = handler;

        /* Configure the interrupt as level or edge. */
        if (level) {
            this->interrupt_controller.SetLevel(irq);
        } else {
            this->interrupt_controller.SetEdge(irq);
        }

        /* Configure the interrupt. */
        this->interrupt_controller.Clear(irq);
        this->interrupt_controller.SetTarget(irq, core_id);
        this->interrupt_controller.SetPriorityLevel(irq, priority);
        this->interrupt_controller.Enable(irq);

        return ResultSuccess();
    }

    Result KInterruptManager::BindLocal(KInterruptHandler *handler, s32 irq, s32 priority, bool manual_clear) {
        /* Ensure the priority level is valid. */
        R_UNLESS(KInterruptController::PriorityLevel_High <= priority, svc::ResultOutOfRange());
        R_UNLESS(priority <= KInterruptController::PriorityLevel_Low,  svc::ResultOutOfRange());

        /* Ensure we aren't already bound. */
        auto &entry = this->GetLocalInterruptEntry(irq);
        R_UNLESS(entry.handler == nullptr, svc::ResultBusy());

        /* Set entry fields. */
        entry.needs_clear = false;
        entry.manually_cleared = manual_clear;
        entry.handler = handler;
        entry.priority = static_cast<u8>(priority);

        /* Configure the interrupt. */
        this->interrupt_controller.Clear(irq);
        this->interrupt_controller.SetPriorityLevel(irq, priority);
        this->interrupt_controller.Enable(irq);

        return ResultSuccess();
    }

    Result KInterruptManager::UnbindGlobal(s32 irq) {
        for (size_t core_id = 0; core_id < cpu::NumCores; core_id++) {
            this->interrupt_controller.ClearTarget(irq, static_cast<s32>(core_id));
        }
        this->interrupt_controller.SetPriorityLevel(irq, KInterruptController::PriorityLevel_Low);
        this->interrupt_controller.Disable(irq);

        GetGlobalInterruptEntry(irq).handler = nullptr;

        return ResultSuccess();
    }

    Result KInterruptManager::UnbindLocal(s32 irq) {
        auto &entry = this->GetLocalInterruptEntry(irq);
        R_UNLESS(entry.handler != nullptr, svc::ResultInvalidState());

        this->interrupt_controller.SetPriorityLevel(irq, KInterruptController::PriorityLevel_Low);
        this->interrupt_controller.Disable(irq);

        entry.handler = nullptr;

        return ResultSuccess();
    }

    Result KInterruptManager::ClearGlobal(s32 irq) {
        /* We can't clear an entry with no handler. */
        auto &entry = GetGlobalInterruptEntry(irq);
        R_UNLESS(entry.handler != nullptr, svc::ResultInvalidState());

        /* If auto-cleared, we can succeed immediately. */
        R_SUCCEED_IF(!entry.manually_cleared);
        R_SUCCEED_IF(!entry.needs_clear);

        /* Clear and enable. */
        entry.needs_clear = false;
        this->interrupt_controller.Enable(irq);
        return ResultSuccess();
    }

    Result KInterruptManager::ClearLocal(s32 irq) {
        /* We can't clear an entry with no handler. */
        auto &entry = this->GetLocalInterruptEntry(irq);
        R_UNLESS(entry.handler != nullptr, svc::ResultInvalidState());

        /* If auto-cleared, we can succeed immediately. */
        R_SUCCEED_IF(!entry.manually_cleared);
        R_SUCCEED_IF(!entry.needs_clear);

        /* Clear and set priority. */
        entry.needs_clear = false;
        this->interrupt_controller.SetPriorityLevel(irq, entry.priority);
        return ResultSuccess();
    }

}
