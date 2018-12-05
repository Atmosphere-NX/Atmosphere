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

#include "atmosphere_test_service.hpp"
#include "atmosphere_test_task.hpp"

TmaTask *AtmosphereTestService::NewTask(TmaPacket *packet) {
    auto new_task = new AtmosphereTestTask(this->manager);
    new_task->SetServiceId(this->GetServiceId());
    new_task->SetTaskId(packet->GetTaskId());
    new_task->OnStart(packet);
    new_task->SetNeedsPackets(true);
    
    return new_task;
}
