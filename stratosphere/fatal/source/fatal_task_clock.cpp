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
#include <stratosphere.hpp>
#include "fatal_task_clock.hpp"

namespace ams::fatal::srv {


    namespace {

        /* Task definition. */
        class AdjustClockTask : public ITaskWithDefaultStack {
            private:
                Result AdjustClockForModule(PcvModule module, u32 hz);
                Result AdjustClock();
            public:
                virtual Result Run() override;
                virtual const char *GetName() const override {
                    return "AdjustClockTask";
                }
        };

        /* Task global. */
        AdjustClockTask g_adjust_clock_task;

        /* Task implementation. */
        Result AdjustClockTask::AdjustClockForModule(PcvModule module, u32 hz) {
            if (hos::GetVersion() >= hos::Version_8_0_0) {
                /* On 8.0.0+, convert to module id + use clkrst API. */
                PcvModuleId module_id;
                R_TRY(pcvGetModuleId(&module_id, module));

                ClkrstSession session;
                R_TRY(clkrstOpenSession(&session, module_id, 3));
                ON_SCOPE_EXIT { clkrstCloseSession(&session); };

                R_TRY(clkrstSetClockRate(&session, hz));
            } else {
                /* On 1.0.0-7.0.1, use pcv API. */
                R_TRY(pcvSetClockRate(module, hz));
            }

            return ResultSuccess();
        }

        Result AdjustClockTask::AdjustClock() {
            /* Fatal sets the CPU to 1020MHz, the GPU to 307 MHz, and the EMC to 1331MHz. */
            constexpr u32 CPU_CLOCK_1020MHZ = 0x3CCBF700L;
            constexpr u32 GPU_CLOCK_307MHZ = 0x124F8000L;
            constexpr u32 EMC_CLOCK_1331MHZ = 0x4F588000L;

            R_TRY(AdjustClockForModule(PcvModule_CpuBus, CPU_CLOCK_1020MHZ));
            R_TRY(AdjustClockForModule(PcvModule_GPU,    GPU_CLOCK_307MHZ));
            R_TRY(AdjustClockForModule(PcvModule_EMC,    EMC_CLOCK_1331MHZ));

            return ResultSuccess();
        }

        Result AdjustClockTask::Run() {
            return AdjustClock();
        }

    }

    ITask *GetAdjustClockTask(const ThrowContext *ctx) {
        g_adjust_clock_task.Initialize(ctx);
        return &g_adjust_clock_task;
    }

}
