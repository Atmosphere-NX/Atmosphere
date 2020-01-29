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
#include <mesosphere/kern_k_spin_lock.hpp>
#include <mesosphere/kern_k_interrupt_task.hpp>
#include <mesosphere/kern_k_timer_task.hpp>
#include <mesosphere/kern_select_interrupt_manager.hpp>

namespace ams::kern {

    class KHardwareTimerBase : public KInterruptTask {
        private:
            using TimerTaskTree = util::IntrusiveRedBlackTreeBaseTraits<KTimerTask>::TreeType<KTimerTask>;
        private:
            KSpinLock lock;
            TimerTaskTree task_tree;
            KTimerTask *next_task;
        public:
            constexpr KHardwareTimerBase() : lock(), task_tree(), next_task(nullptr) { /* ... */ }

            virtual KInterruptTask *OnInterrupt(s32 interrupt_id) override { return this; }
        protected:
            KSpinLock &GetLock() { return this->lock; }

            /* TODO: Actually implement more of KHardwareTimerBase */
    };

}
