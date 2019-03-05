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

void TmaTask::SetNeedsPackets(bool n) {
    this->needs_packets = n;
    this->manager->Tick();
}

TmaPacket *TmaTask::AllocateSendPacket(bool continuation) {
    auto packet = this->manager->AllocateSendPacket();
    packet->SetServiceId(this->service_id);
    packet->SetTaskId(this->task_id);
    packet->SetCommand(this->command);
    packet->SetContinuation(continuation);

    return packet;
}


void TmaTask::FreePacket(TmaPacket *packet) {
    this->manager->FreePacket(packet);
}

void TmaTask::Complete() {
    SetNeedsPackets(false);
    this->state = TmaTaskState::Complete;
    this->manager->Tick();
}

void TmaTask::Cancel() {
    SetNeedsPackets(false);
    this->state = TmaTaskState::Canceled;
    this->manager->Tick();
}