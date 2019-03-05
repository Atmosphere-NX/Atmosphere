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

#include "atmosphere_test_task.hpp"

void AtmosphereTestTask::OnStart(TmaPacket *packet) {
    packet->Read<u32>(this->arg);
}

void AtmosphereTestTask::OnReceivePacket(TmaPacket *packet) {
    std::abort();
}

void AtmosphereTestTask::OnSendPacket(TmaPacket *packet) {
    for (size_t i = 0; i < this->arg && i < 0x100; i++) {
        packet->Write<u8>('A');
    }
    
    this->Complete();
}