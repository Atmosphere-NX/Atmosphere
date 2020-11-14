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
        this->interrupt_id = interrupt_name;

        /* Initialize readable event base. */
        KReadableEvent::Initialize(nullptr);

        /* Try to register the task. */
        R_TRY(KInterruptEventTask::Register(std::addressof(this->task), this->interrupt_id, type == ams::svc::InterruptType_Level, this));

        /* Mark initialized. */
        this->is_initialized = true;
        return ResultSuccess();
    }

    void KInterruptEvent::Finalize() {
        MESOSPHERE_ASSERT_THIS();

        MESOSPHERE_ASSERT(this->task != nullptr);
        this->task->Unregister();
    }

    Result KInterruptEvent::Reset() {
        MESOSPHERE_ASSERT_THIS();

        /* Lock the task table. */
        KScopedLightLock lk(g_interrupt_event_lock);

        /* Clear the event. */
        R_TRY(KReadableEvent::Reset());

        /* Clear the interrupt. */
        Kernel::GetInterruptManager().ClearInterrupt(this->interrupt_id);

        return ResultSuccess();
    }

    Result KInterruptEventTask::Register(KInterruptEventTask **out, s32 interrupt_id, bool level, KInterruptEvent *event) {
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
            R_UNLESS(task->event == nullptr, svc::ResultBusy());
        } else {
            /* Allocate a new task. */
            task = KInterruptEventTask::Allocate();
            R_UNLESS(task != nullptr, svc::ResultOutOfResource());

            allocated = true;
        }

        /* Register/bind the interrupt task. */
        {
            /* Ensure that the task is cleaned up if anything goes wrong. */
            auto task_guard = SCOPE_GUARD { if (allocated) { KInterruptEventTask::Free(task); } };

            /* Bind the interrupt handler. */
            R_TRY(Kernel::GetInterruptManager().BindHandler(task, interrupt_id, GetCurrentCoreId(), KInterruptController::PriorityLevel_High, true, level));

            /* We successfully registered, so we don't need to free the task. */
            task_guard.Cancel();
        }

        /* Set the event. */
        task->event = event;

        /* If we allocated, set the event in the table. */
        if (allocated) {
            task->interrupt_id = interrupt_id;
            g_interrupt_event_task_table[interrupt_id] = task;
        }

        /* Set the output. */
        *out = task;
        return ResultSuccess();
    }

    void KInterruptEventTask::Unregister() {
        MESOSPHERE_ASSERT_THIS();

        /* Lock the task table. */
        KScopedLightLock lk(g_interrupt_event_lock);

        /* Ensure we can unregister. */
        MESOSPHERE_ABORT_UNLESS(g_interrupt_event_task_table[this->interrupt_id] == this);
        MESOSPHERE_ABORT_UNLESS(this->event != nullptr);
        this->event = nullptr;

        /* Unbind the interrupt. */
        Kernel::GetInterruptManager().UnbindHandler(this->interrupt_id, GetCurrentCoreId());
    }

    KInterruptTask *KInterruptEventTask::OnInterrupt(s32 interrupt_id) {
        MESOSPHERE_ASSERT_THIS();
        MESOSPHERE_UNUSED(interrupt_id);
        return this;
    }

    void KInterruptEventTask::DoTask() {
        MESOSPHERE_ASSERT_THIS();

        /* Lock the task table. */
        KScopedLightLock lk(g_interrupt_event_lock);

        if (this->event != nullptr) {
            this->event->Signal();
        }
    }
}
