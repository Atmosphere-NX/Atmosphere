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

namespace ams::kern {

    Result KInterruptEvent::Initialize(int32_t interrupt_name, ams::svc::InterruptType type) {
        MESOSPHERE_ASSERT_THIS();

        /* Verify the interrupt is defined and global. */
        R_UNLESS(Kernel::GetInterruptManager().IsInterruptDefined(interrupt_name), svc::ResultOutOfRange());
        R_UNLESS(Kernel::GetInterruptManager().IsGlobal(interrupt_name),           svc::ResultOutOfRange());

        /* Set interrupt id. */
        m_interrupt_id = interrupt_name;

        /* Set core id. */
        m_core_id = GetCurrentCoreId();

        /* Initialize readable event base. */
        KReadableEvent::Initialize(nullptr);

        /* Bind ourselves as the handler for our interrupt id. */
        R_TRY(Kernel::GetInterruptManager().BindHandler(this, m_interrupt_id, m_core_id, KInterruptController::PriorityLevel_High, true, type == ams::svc::InterruptType_Level));

        /* Mark initialized. */
        m_is_initialized = true;
        R_SUCCEED();
    }

    void KInterruptEvent::Finalize() {
        MESOSPHERE_ASSERT_THIS();

        /* Unbind ourselves as the handler for our interrupt id. */
        Kernel::GetInterruptManager().UnbindHandler(m_interrupt_id, m_core_id);

        /* Synchronize the unbind on all cores, before proceeding. */
        KDpcManager::Sync();

        /* Perform inherited finalization. */
        KReadableEvent::Finalize();
    }

    Result KInterruptEvent::Reset() {
        MESOSPHERE_ASSERT_THIS();

        /* Lock the scheduler. */
        KScopedSchedulerLock sl;

        /* Clear the event. */
        R_TRY(KReadableEvent::Reset());

        /* Clear the interrupt. */
        Kernel::GetInterruptManager().ClearInterrupt(m_interrupt_id, m_core_id);

        R_SUCCEED();
    }

    KInterruptTask *KInterruptEvent::OnInterrupt(s32 interrupt_id) {
        MESOSPHERE_ASSERT_THIS();
        MESOSPHERE_UNUSED(interrupt_id);
        return this;
    }

    void KInterruptEvent::DoTask() {
        MESOSPHERE_ASSERT_THIS();

        /* Lock the scheduler. */
        KScopedSchedulerLock sl;

        /* Signal. */
        this->Signal();
    }
}
