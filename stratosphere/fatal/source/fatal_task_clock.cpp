/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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
#include "fatal_task_clock.hpp"

Result AdjustClockTask::AdjustClockForModule(PcvModule module, u32 hz) {
    Result rc;

    if (GetRuntimeFirmwareVersion() >= FirmwareVersion_800) {
        /* On 8.0.0+, convert to module id + use clkrst API. */
        PcvModuleId module_id;
        if (R_FAILED((rc = pcvGetModuleId(&module_id, module)))) {
            return rc;
        }

        ClkrstSession session;
        Result rc = clkrstOpenSession(&session, module_id, 3);
        if (R_FAILED(rc)) {
            return rc;
        }
        ON_SCOPE_EXIT { clkrstCloseSession(&session); };

        rc = clkrstSetClockRate(&session, hz);
    } else {
        /* On 1.0.0-7.0.1, use pcv API. */
        rc = pcvSetClockRate(module, hz);
    }

    return rc;
}

Result AdjustClockTask::AdjustClock() {
    /* Fatal sets the CPU to 1020MHz, the GPU to 307 MHz, and the EMC to 1331MHz. */
    constexpr u32 CPU_CLOCK_1020MHZ = 0x3CCBF700L;
    constexpr u32 GPU_CLOCK_307MHZ = 0x124F8000L;
    constexpr u32 EMC_CLOCK_1331MHZ = 0x4F588000L;
    Result rc = ResultSuccess;

    if (R_FAILED((rc = AdjustClockForModule(PcvModule_CpuBus, CPU_CLOCK_1020MHZ)))) {
        return rc;
    }

    if (R_FAILED((rc = AdjustClockForModule(PcvModule_GPU, GPU_CLOCK_307MHZ)))) {
        return rc;
    }

    if (R_FAILED((rc = AdjustClockForModule(PcvModule_EMC, EMC_CLOCK_1331MHZ)))) {
        return rc;
    }

    return rc;
}

Result AdjustClockTask::Run() {
    return AdjustClock();
}