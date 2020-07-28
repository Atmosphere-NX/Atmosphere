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
#include <mesosphere/kern_common.hpp>
#include <mesosphere/kern_k_auto_object.hpp>
#include <mesosphere/kern_slab_helpers.hpp>
#include <mesosphere/kern_k_readable_event.hpp>
#include <mesosphere/kern_k_interrupt_task.hpp>

namespace ams::kern {

    class KInterruptEventTask;

    class KInterruptEvent final : public KAutoObjectWithSlabHeapAndContainer<KInterruptEvent, KReadableEvent> {
        MESOSPHERE_AUTOOBJECT_TRAITS(KInterruptEvent, KReadableEvent);
        private:
            KInterruptEventTask *task;
            s32 interrupt_id;
            bool is_initialized;
        public:
            constexpr KInterruptEvent() : task(nullptr), interrupt_id(-1), is_initialized(false) { /* ... */ }
            virtual ~KInterruptEvent() { /* ... */ }

            Result Initialize(int32_t interrupt_name, ams::svc::InterruptType type);
            virtual void Finalize() override;

            virtual Result Reset() override;

            virtual bool IsInitialized() const override { return this->is_initialized; }

            static void PostDestroy(uintptr_t arg) { /* ... */ }

            constexpr s32 GetInterruptId() const { return this->interrupt_id; }
    };

    class KInterruptEventTask : public KSlabAllocated<KInterruptEventTask>, public KInterruptTask {
        private:
            KInterruptEvent *event;
            s32 interrupt_id;
        public:
            constexpr KInterruptEventTask() : event(nullptr), interrupt_id(-1) { /* ... */ }
            ~KInterruptEventTask() { /* ... */ }

            virtual KInterruptTask *OnInterrupt(s32 interrupt_id) override;
            virtual void DoTask() override;

            void Unregister();
        public:
            static Result Register(KInterruptEventTask **out, s32 interrupt_id, bool level, KInterruptEvent *event);
    };

}
