/*
 * Copyright (c) 2018 Atmosphère-NX
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

#include <switch.h>
#include <stratosphere.hpp>
#include "tma_task.hpp"
#include "tma_service_manager.hpp"

void TmaTask::Complete() {
    /* TODO: Set packet state */
    this->state = TmaTaskState::Complete;
    this->manager->Tick();
}

void TmaTask::Cancel() {
    /* TODO: Set packet state */
    this->state = TmaTaskState::Canceled;
    this->manager->Tick();
}