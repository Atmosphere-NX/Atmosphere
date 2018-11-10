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
#include "fatal_task_clock.hpp"


Result AdjustClockTask::AdjustClock() {
    /* Fatal sets the CPU to 1020MHz, the GPU to 307 MHz, and the EMC to 1331MHz. */
    constexpr u32 CPU_CLOCK_1020MHZ = 0x3CCBF700L;
    constexpr u32 GPU_CLOCK_307MHZ = 0x124F8000L;
    constexpr u32 EMC_CLOCK_1331MHZ = 0x4F588000L;
    Result rc = 0;
    
    if (R_FAILED((rc = pcvSetClockRate(PcvModule_Cpu, CPU_CLOCK_1020MHZ)))) {
        return rc;
    }
    
    if (R_FAILED((rc = pcvSetClockRate(PcvModule_Gpu, GPU_CLOCK_307MHZ)))) {
        return rc;
    }
    
    if (R_FAILED((rc = pcvSetClockRate(PcvModule_Emc, EMC_CLOCK_1331MHZ)))) {
        return rc;
    }
    
    return rc;
}

Result AdjustClockTask::Run() {
    return AdjustClock();
}