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
#include <vapours.hpp>

namespace ams::gic {

    enum InterruptMode {
        InterruptMode_Level = 0,
        InterruptMode_Edge  = 1,
    };

    constexpr inline s32 HighestPriority = 0;
    constexpr inline s32 InterruptCount  = 224;

    void SetRegisterAddress(uintptr_t distributor_address, uintptr_t cpu_interface_address);
    void InitializeCommon();
    void InitializeCoreUnique();

    void SetPriority(int interrupt_id, int priority);
    void SetInterruptGroup(int interrupt_id, int group);
    void SetEnable(int interrupt_id, bool enable);
    void SetSpiTargetCpu(int interrupt_id, u32 cpu_mask);
    void SetSpiMode(int interrupt_id, InterruptMode mode);

    void SetPending(int interrupt_id);

    int GetInterruptRequestId();
    void SetEndOfInterrupt(int interrupt_id);

}