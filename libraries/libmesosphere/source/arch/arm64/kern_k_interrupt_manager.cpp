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
#include <mesosphere.hpp>

namespace ams::kern::arch::arm64 {

    void KInterruptManager::Initialize(s32 core_id) {
        m_interrupt_controller.Initialize(core_id);
    }

    void KInterruptManager::Finalize(s32 core_id) {
        m_interrupt_controller.Finalize(core_id);
    }

    void KInterruptManager::Save(s32 core_id) {
        /* Verify core id. */
        MESOSPHERE_ASSERT(core_id == GetCurrentCoreId());

        /* Ensure all cores get to this point before continuing. */
        cpu::SynchronizeAllCores();

        /* If on core 0, save the global interrupts. */
        if (core_id == 0) {
            MESOSPHERE_ABORT_UNLESS(!m_global_state_saved);
            m_interrupt_controller.SaveGlobal(std::addressof(m_global_state));
            m_global_state_saved = true;
        }

        /* Ensure all cores get to this point before continuing. */
        cpu::SynchronizeAllCores();

        /* Save all local interrupts. */
        MESOSPHERE_ABORT_UNLESS(!m_local_state_saved[core_id]);
        m_interrupt_controller.SaveCoreLocal(std::addressof(m_local_states[core_id]));
        m_local_state_saved[core_id] = true;

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
        MESOSPHERE_ASSERT(m_local_state_saved[core_id]);
        m_interrupt_controller.RestoreCoreLocal(std::addressof(m_local_states[core_id]));
        m_local_state_saved[core_id] = false;

        /* Ensure all cores get to this point before continuing. */
        cpu::SynchronizeAllCores();

        /* If on core 0, restore the global interrupts. */
        if (core_id == 0) {
            MESOSPHERE_ASSERT(m_global_state_saved);
            m_interrupt_controller.RestoreGlobal(std::addressof(m_global_state));
            m_global_state_saved = false;
        }

        /* Ensure all cores get to this point before continuing. */
        cpu::SynchronizeAllCores();
    }

    bool KInterruptManager::OnHandleInterrupt() {
        /* Get the interrupt id. */
        const u32 raw_irq = m_interrupt_controller.GetIrq();
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
                    m_interrupt_controller.Disable(irq);
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
                    m_interrupt_controller.Disable(irq);
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
        m_interrupt_controller.EndOfInterrupt(raw_irq);

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
            if (user_mode) {
                /* If the interrupt occurred in the middle of a userland cache maintenance operation, ensure memory consistency before rescheduling. */
                if (GetCurrentThread().IsInUserCacheMaintenanceOperation()) {
                    cpu::DataSynchronizationBarrier();
                }

                /* If the user disable count is set, we may need to pin the current thread. */
                if (GetCurrentThread().GetUserDisableCount() != 0 && GetCurrentProcess().GetPinnedThread(GetCurrentCoreId()) == nullptr) {
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
            } else {
                /* If the interrupt occurred in the middle of a cache maintenance operation, ensure memory consistency before rescheduling. */
                if (GetCurrentThread().IsInCacheMaintenanceOperation()) {
                    cpu::DataSynchronizationBarrier();
                } else if (GetCurrentThread().IsInTlbMaintenanceOperation()) {
                    /* Otherwise, if we're in the middle of a tlb maintenance operation, ensure inner shareable memory consistency before rescheduling. */
                    cpu::DataSynchronizationBarrierInnerShareable();
                }

                /* Request interrupt scheduling. */
                Kernel::GetScheduler().RequestScheduleOnInterrupt();
            }
        }

        /* If user mode, check if the thread needs termination. */
        /* If it does, we can take advantage of this to terminate it. */
        if (user_mode) {
            KThread *cur_thread = GetCurrentThreadPointer();
            if (cur_thread->IsTerminationRequested()) {
                EnableInterrupts();
                cur_thread->Exit();
            }
        }
    }

    Result KInterruptManager::BindHandler(KInterruptHandler *handler, s32 irq, s32 core_id, s32 priority, bool manual_clear, bool level) {
        MESOSPHERE_UNUSED(core_id);

        R_UNLESS(KInterruptController::IsGlobal(irq) || KInterruptController::IsLocal(irq), svc::ResultOutOfRange());

        if (KInterruptController::IsGlobal(irq)) {
            KScopedInterruptDisable di;
            KScopedSpinLock lk(this->GetGlobalInterruptLock());
            R_RETURN(this->BindGlobal(handler, irq, core_id, priority, manual_clear, level));
        } else {
            MESOSPHERE_ASSERT(core_id == GetCurrentCoreId());

            KScopedInterruptDisable di;
            R_RETURN(this->BindLocal(handler, irq, priority, manual_clear));
        }
    }

    void KInterruptManager::UnbindHandler(s32 irq, s32 core_id) {
        MESOSPHERE_UNUSED(core_id);

        MESOSPHERE_ASSERT(KInterruptController::IsGlobal(irq) || KInterruptController::IsLocal(irq));


        if (KInterruptController::IsGlobal(irq)) {
            KScopedInterruptDisable di;

            KScopedSpinLock lk(this->GetGlobalInterruptLock());
            return this->UnbindGlobal(irq);
        } else if (KInterruptController::IsLocal(irq)) {
            MESOSPHERE_ASSERT(core_id == GetCurrentCoreId());

            KScopedInterruptDisable di;
            return this->UnbindLocal(irq);
        }
    }

    void KInterruptManager::ClearInterrupt(s32 irq, s32 core_id) {
        MESOSPHERE_UNUSED(core_id);

        MESOSPHERE_ASSERT(KInterruptController::IsGlobal(irq) || KInterruptController::IsLocal(irq));


        if (KInterruptController::IsGlobal(irq)) {
            KScopedInterruptDisable di;
            KScopedSpinLock lk(this->GetGlobalInterruptLock());
            return this->ClearGlobal(irq);
        } else if (KInterruptController::IsLocal(irq)) {
            MESOSPHERE_ASSERT(core_id == GetCurrentCoreId());

            KScopedInterruptDisable di;
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
            m_interrupt_controller.SetLevel(irq);
        } else {
            m_interrupt_controller.SetEdge(irq);
        }

        /* Configure the interrupt. */
        m_interrupt_controller.Clear(irq);
        m_interrupt_controller.SetTarget(irq, core_id);
        m_interrupt_controller.SetPriorityLevel(irq, priority);
        m_interrupt_controller.Enable(irq);

        R_SUCCEED();
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
        m_interrupt_controller.Clear(irq);
        m_interrupt_controller.SetPriorityLevel(irq, priority);
        m_interrupt_controller.Enable(irq);

        R_SUCCEED();
    }

    void KInterruptManager::UnbindGlobal(s32 irq) {
        for (size_t core_id = 0; core_id < cpu::NumCores; core_id++) {
            m_interrupt_controller.ClearTarget(irq, static_cast<s32>(core_id));
        }
        m_interrupt_controller.SetPriorityLevel(irq, KInterruptController::PriorityLevel_Low);
        m_interrupt_controller.Disable(irq);

        GetGlobalInterruptEntry(irq).handler = nullptr;
    }

    void KInterruptManager::UnbindLocal(s32 irq) {
        m_interrupt_controller.SetPriorityLevel(irq, KInterruptController::PriorityLevel_Low);
        m_interrupt_controller.Disable(irq);

        this->GetLocalInterruptEntry(irq).handler = nullptr;
    }

    void KInterruptManager::ClearGlobal(s32 irq) {
        /* Get the entry. */
        auto &entry = GetGlobalInterruptEntry(irq);

        /* If not auto-cleared, clear and enable. */
        if (entry.manually_cleared && entry.needs_clear) {
            entry.needs_clear = false;
            m_interrupt_controller.Enable(irq);
        }
    }

    void KInterruptManager::ClearLocal(s32 irq) {
        /* Get the entry. */
        auto &entry = this->GetLocalInterruptEntry(irq);

        /* If not auto-cleared, clear and enable. */
        if (entry.manually_cleared && entry.needs_clear) {
            entry.needs_clear = false;
            m_interrupt_controller.Enable(irq);
        }
    }

}
