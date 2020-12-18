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

namespace ams::kern {

    namespace {

        constinit KLightLock g_interrupt_event_lock;
        constinit KInterruptEventTask *g_interrupt_event_task_table[KInterruptController::NumInterrupts] = {};

    }

    Result KInterruptEvent::Initialize(int32_t interrupt_name, ams::svc::InterruptType type) {
        MESOSPHERE_ASSERT_THIS();

        /* Set interrupt id. */
        m_interrupt_id = interrupt_name;

        /* Initialize readable event base. */
        KReadableEvent::Initialize(nullptr);

        /* Try to register the task. */
        R_TRY(KInterruptEventTask::Register(m_interrupt_id, type == ams::svc::InterruptType_Level, this));

        /* Mark initialized. */
        m_is_initialized = true;
        return ResultSuccess();
    }

    void KInterruptEvent::Finalize() {
        MESOSPHERE_ASSERT_THIS();

        g_interrupt_event_task_table[m_interrupt_id]->Unregister(m_interrupt_id);

        /* Perform inherited finalization. */
        KAutoObjectWithSlabHeapAndContainer<KInterruptEvent, KReadableEvent>::Finalize();
    }

    Result KInterruptEvent::Reset() {
        MESOSPHERE_ASSERT_THIS();

        /* Lock the task. */
        KScopedLightLock lk(g_interrupt_event_task_table[m_interrupt_id]->GetLock());

        /* Clear the event. */
        R_TRY(KReadableEvent::Reset());

        /* Clear the interrupt. */
        Kernel::GetInterruptManager().ClearInterrupt(m_interrupt_id);

        return ResultSuccess();
    }

    Result KInterruptEventTask::Register(s32 interrupt_id, bool level, KInterruptEvent *event) {
        /* Verify the interrupt id is defined and global. */
        R_UNLESS(Kernel::GetInterruptManager().IsInterruptDefined(interrupt_id), svc::ResultOutOfRange());
        R_UNLESS(Kernel::GetInterruptManager().IsGlobal(interrupt_id),           svc::ResultOutOfRange());

        /* Lock the task table. */
        KScopedLightLock lk(g_interrupt_event_lock);

        /* Get a task for the id. */
        bool allocated = false;
        KInterruptEventTask *task = g_interrupt_event_task_table[interrupt_id];
        if (task != nullptr) {
            /* Check that there's not already an event for this task. */
            R_UNLESS(task->m_event == nullptr, svc::ResultBusy());
        } else {
            /* Allocate a new task. */
            task = KInterruptEventTask::Allocate();
            R_UNLESS(task != nullptr, svc::ResultOutOfResource());

            allocated = true;
        }

        /* Ensure that the task is cleaned up if anything goes wrong. */
        auto task_guard = SCOPE_GUARD { if (allocated) { KInterruptEventTask::Free(task); } };

        /* Register/bind the interrupt task. */
        {
            /* Acqquire exclusive access to the task. */
            KScopedLightLock tlk(task->m_lock);

            /* Bind the interrupt handler. */
            R_TRY(Kernel::GetInterruptManager().BindHandler(task, interrupt_id, GetCurrentCoreId(), KInterruptController::PriorityLevel_High, true, level));

            /* Set the event. */
            task->m_event = event;
        }

        /* If we allocated, set the event in the table. */
        if (allocated) {
            g_interrupt_event_task_table[interrupt_id] = task;
        }

        /* We successfully registered, so we don't need to free the task. */
        task_guard.Cancel();
        return ResultSuccess();
    }

    void KInterruptEventTask::Unregister(s32 interrupt_id) {
        MESOSPHERE_ASSERT_THIS();

        /* Lock the task table. */
        KScopedLightLock lk(g_interrupt_event_lock);

        /* Lock the task. */
        KScopedLightLock tlk(m_lock);

        /* Ensure we can unregister. */
        MESOSPHERE_ABORT_UNLESS(g_interrupt_event_task_table[interrupt_id] == this);
        MESOSPHERE_ABORT_UNLESS(m_event != nullptr);

        /* Unbind the interrupt. */
        m_event = nullptr;
        Kernel::GetInterruptManager().UnbindHandler(interrupt_id, GetCurrentCoreId());
    }

    KInterruptTask *KInterruptEventTask::OnInterrupt(s32 interrupt_id) {
        MESOSPHERE_ASSERT_THIS();
        MESOSPHERE_UNUSED(interrupt_id);
        return this;
    }

    void KInterruptEventTask::DoTask() {
        MESOSPHERE_ASSERT_THIS();

        /* Lock the task table. */
        KScopedLightLock lk(m_lock);

        if (m_event != nullptr) {
            m_event->Signal();
        }
    }
}
