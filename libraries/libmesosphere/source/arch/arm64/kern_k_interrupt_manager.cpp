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

    void KInterruptManager::Initialize(s32 core_id) {
        this->interrupt_controller.Initialize(core_id);
    }

    void KInterruptManager::Finalize(s32 core_id) {
        this->interrupt_controller.Finalize(core_id);
    }

    void KInterruptManager::Save(s32 core_id) {
        /* Verify core id. */
        MESOSPHERE_ASSERT(core_id == GetCurrentCoreId());

        /* Ensure all cores get to this point before continuing. */
        cpu::SynchronizeAllCores();

        /* If on core 0, save the global interrupts. */
        if (core_id == 0) {
            MESOSPHERE_ABORT_UNLESS(!this->global_state_saved);
            this->interrupt_controller.SaveGlobal(std::addressof(this->global_state));
            this->global_state_saved = true;
        }

        /* Ensure all cores get to this point before continuing. */
        cpu::SynchronizeAllCores();

        /* Save all local interrupts. */
        MESOSPHERE_ABORT_UNLESS(!this->local_state_saved[core_id]);
        this->interrupt_controller.SaveCoreLocal(std::addressof(this->local_states[core_id]));
        this->local_state_saved[core_id] = true;

        /* Ensure all cores get to this point before continuing. */
        cpu::SynchronizeAllCores();

        /* Finalize all cores other than core 0. */
        if (core_id != 0) {
            this->Finalize(core_id);
        }

        /* Ensure all cores get to this point before continuing. */
        cpu::SynchronizeAllCores();

        /* Finalize core 0. */
        if (core_id == 0) {
            this->Finalize(core_id);
        }
    }

    void KInterruptManager::Restore(s32 core_id) {
        /* Verify core id. */
        MESOSPHERE_ASSERT(core_id == GetCurrentCoreId());

        /* Ensure all cores get to this point before continuing. */
        cpu::SynchronizeAllCores();

        /* Initialize core 0. */
        if (core_id == 0) {
            this->Initialize(core_id);
        }

        /* Ensure all cores get to this point before continuing. */
        cpu::SynchronizeAllCores();

        /* Initialize all cores other than core 0. */
        if (core_id != 0) {
            this->Initialize(core_id);
        }

        /* Ensure all cores get to this point before continuing. */
        cpu::SynchronizeAllCores();

        /* Restore all local interrupts. */
        MESOSPHERE_ASSERT(this->local_state_saved[core_id]);
        this->interrupt_controller.RestoreCoreLocal(std::addressof(this->local_states[core_id]));
        this->local_state_saved[core_id] = false;

        /* Ensure all cores get to this point before continuing. */
        cpu::SynchronizeAllCores();

        /* If on core 0, restore the global interrupts. */
        if (core_id == 0) {
            MESOSPHERE_ASSERT(this->global_state_saved);
            this->interrupt_controller.RestoreGlobal(std::addressof(this->global_state));
            this->global_state_saved = false;
        }

        /* Ensure all cores get to this point before continuing. */
        cpu::SynchronizeAllCores();
    }

    bool KInterruptManager::OnHandleInterrupt() {
        /* Get the interrupt id. */
        const u32 raw_irq = this->interrupt_controller.GetIrq();
        const s32 irq = KInterruptController::ConvertRawIrq(raw_irq);

        /* Trace the interrupt. */
        MESOSPHERE_KTRACE_INTERRUPT(irq);

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
            KScopedSpinLock lk(this->GetGlobalInterruptLock());

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
            /* If the user disable count is set, we may need to pin the current thread. */
            if (user_mode && GetCurrentThread().GetUserDisableCount() != 0 && GetCurrentProcess().GetPinnedThread(GetCurrentCoreId()) == nullptr) {
                KScopedSchedulerLock sl;

                /* Pin the current thread. */
                GetCurrentProcess().PinCurrentThread();

                /* Set the interrupt flag for the thread. */
                GetCurrentThread().SetInterruptFlag();

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
            KScopedSpinLock lk(this->GetGlobalInterruptLock());
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
            KScopedSpinLock lk(this->GetGlobalInterruptLock());
            return this->UnbindGlobal(irq);
        } else {
            MESOSPHERE_ASSERT(core_id == GetCurrentCoreId());
            return this->UnbindLocal(irq);
        }
    }

    Result KInterruptManager::ClearInterrupt(s32 irq) {
        R_UNLESS(KInterruptController::IsGlobal(irq), svc::ResultOutOfRange());

        KScopedInterruptDisable di;
        KScopedSpinLock lk(this->GetGlobalInterruptLock());
        return this->ClearGlobal(irq);
    }

    Result KInterruptManager::ClearInterrupt(s32 irq, s32 core_id) {
        R_UNLESS(KInterruptController::IsGlobal(irq) || KInterruptController::IsLocal(irq), svc::ResultOutOfRange());

        KScopedInterruptDisable di;

        if (KInterruptController::IsGlobal(irq)) {
            KScopedSpinLock lk(this->GetGlobalInterruptLock());
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
