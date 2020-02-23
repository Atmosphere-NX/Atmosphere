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
#include <mesosphere/kern_k_timer_task.hpp>
#include <mesosphere/kern_k_thread.hpp>

namespace ams::kern {

    class KWaitObject : public KTimerTask {
        private:
            using Entry = KThread::QueueEntry;
        private:
            Entry root;
            bool  uses_timer;
        public:
            constexpr KWaitObject() : root(), uses_timer() { /* ... */ }

            virtual void OnTimer() override;

            /* TODO: Member functions */
    };

}
