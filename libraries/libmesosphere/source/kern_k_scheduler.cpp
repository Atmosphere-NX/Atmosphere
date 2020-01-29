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

    KScheduler::KScheduler()
        : is_active(false), core_id(0), prev_thread(nullptr), last_context_switch_time(0), idle_thread(nullptr)
    {
        this->state.needs_scheduling = true;
        this->state.interrupt_task_thread_runnable = false;
        this->state.should_count_idle = false;
        this->state.idle_count = 0;
        this->state.idle_thread_stack = nullptr;
        this->state.highest_priority_thread = nullptr;
    }

}
