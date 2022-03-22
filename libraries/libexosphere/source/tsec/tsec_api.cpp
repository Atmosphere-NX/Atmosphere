/*
 * Copyright (c) Atmosphère-NX
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
#include <exosphere.hpp>
#include "tsec_registers.hpp"
#include "../kfuse/kfuse_registers.hpp"

namespace ams::tsec {

    namespace {

        constexpr inline const uintptr_t KFUSE = 0x7000FC00;
        constexpr inline const uintptr_t TSEC  = 0x54500000;

        enum TsecResult : u32 {
            TsecResult_Success = 0xB0B0B0B0,
            TsecResult_Failure = 0xD0D0D0D0,
        };

        enum TsecMemory {
            TsecMemory_Imem,
            TsecMemory_Dmem,
        };

        bool WaitForKfuseReady() {
            constexpr auto KfuseTimeout = 10 * 1000; /* 10 ms. */

            const u32 end_time = util::GetMicroSeconds() + KfuseTimeout;

            /* Wait for STATE_DONE. */
            while (!reg::HasValue(KFUSE + KFUSE_STATE, KFUSE_REG_BITS_ENUM(STATE_DONE, DONE))) {
                if (util::GetMicroSeconds() >= end_time) {
                    return false;
                }
            }

            /* Check for STATE_CRCPASS. */
            return reg::HasValue(KFUSE + KFUSE_STATE, KFUSE_REG_BITS_ENUM(STATE_CRCPASS, PASS));
        }

        void WaitForDmaIdle() {
            constexpr auto DmaTimeout = 10 * 1000 * 1000; /* 10 Seconds. */

            u32 cur_time = util::GetMicroSeconds();
            const u32 end_time = cur_time + DmaTimeout;

            while (cur_time <= end_time) {
                if (reg::HasValue(TSEC + TSEC_FALCON_DMATRFCMD, TSEC_REG_BITS_ENUM(FALCON_DMATRFCMD_BUSY, IDLE))) {
                    return;
                }

                cur_time = util::GetMicroSeconds();
            }

            AMS_ABORT("tsec dma timeout");
        }

        void WaitForTsecIdle() {
            constexpr auto TsecTimeout = 2 * 1000 * 1000; /* 2 Seconds. */

            u32 cur_time = util::GetMicroSeconds();
            const u32 end_time = cur_time + TsecTimeout;

            while (cur_time <= end_time) {
                if (reg::HasValue(TSEC + TSEC_FALCON_CPUCTL, TSEC_REG_BITS_ENUM(FALCON_CPUCTL_HALTED, TRUE))) {
                    return;
                }

                cur_time = util::GetMicroSeconds();
            }

            AMS_ABORT("tsec timeout");
        }

        void DoDma256(TsecMemory memory, u32 dst_offset, u32 src_offset) {
            reg::Write(TSEC + TSEC_FALCON_DMATRFMOFFS, TSEC_REG_BITS_VALUE(FALCON_DMATRFMOFFS_OFFSET, dst_offset));
            reg::Write(TSEC + TSEC_FALCON_DMATRFFBOFFS, src_offset);

            if (memory == TsecMemory_Imem) {
                reg::Write(TSEC + TSEC_FALCON_DMATRFCMD, TSEC_REG_BITS_ENUM(FALCON_DMATRFCMD_TO,   IMEM),
                                                         TSEC_REG_BITS_ENUM(FALCON_DMATRFCMD_SIZE,   4B));
            } else {
                reg::Write(TSEC + TSEC_FALCON_DMATRFCMD, TSEC_REG_BITS_ENUM(FALCON_DMATRFCMD_TO,    DMEM),
                                                         TSEC_REG_BITS_ENUM(FALCON_DMATRFCMD_SIZE,  256B));
            }

            WaitForDmaIdle();
        }

    }

    bool RunTsecFirmware(const void *fw, size_t fw_size) {
        /* Enable relevant clocks. */
        clkrst::EnableHost1xClock();
        clkrst::EnableTsecClock();
        clkrst::EnableSorSafeClock();
        clkrst::EnableSor0Clock();
        clkrst::EnableSor1Clock();
        clkrst::EnableKfuseClock();

        /* Disable clocks once we're done. */
        ON_SCOPE_EXIT {
            clkrst::DisableHost1xClock();
            clkrst::DisableTsecClock();
            clkrst::DisableSorSafeClock();
            clkrst::DisableSor0Clock();
            clkrst::DisableSor1Clock();
            clkrst::DisableKfuseClock();
        };

        /* Wait for kfuse to be ready. */
        if (!WaitForKfuseReady()) {
            return false;
        }

        /* Configure falcon. */
        reg::Write(TSEC + TSEC_FALCON_DMACTL, 0);
        reg::Write(TSEC + TSEC_FALCON_IRQMSET, 0xFFF2);
        reg::Write(TSEC + TSEC_FALCON_IRQDEST, 0xFFF0);
        reg::Write(TSEC + TSEC_FALCON_ITFEN, 0x3);

        /* Wait for TSEC dma to be idle. */
        WaitForDmaIdle();

        /* Set the base address for transfers. */
        reg::Write(TSEC + TSEC_FALCON_DMATRFBASE, reinterpret_cast<uintptr_t>(fw) >> 8);

        /* Transfer all data to TSEC imem. */
        for (size_t i = 0; i < fw_size; i += 0x100) {
            DoDma256(TsecMemory_Imem, i, i);
        }

        /* Write the magic value to host1x syncpoint 160. */
        reg::Write(0x50003300, 0x34C2E1DA);

        /* Execute the firmware. */
        reg::Write(TSEC + TSEC_FALCON_MAILBOX0, 0);
        reg::Write(TSEC + TSEC_FALCON_MAILBOX1, 0);
        reg::Write(TSEC + TSEC_FALCON_BOOTVEC, 0);
        reg::Write(TSEC + TSEC_FALCON_CPUCTL, TSEC_REG_BITS_ENUM(FALCON_CPUCTL_STARTCPU, TRUE));

        /* Wait for TSEC dma to be idle. */
        WaitForDmaIdle();

        /* Wait for TSEC to complete. */
        WaitForTsecIdle();

        /* Clear magic value from host1x syncpoint 160. */
        reg::Write(0x50003300, 0);

        /* Return whether the tsec firmware succeeded. */
        return reg::Read(TSEC + TSEC_FALCON_MAILBOX1) == TsecResult_Success;
    }

    void Lock() {
        /* Set the tsec host1x syncpoint (160) to be secure. */
        /* TODO: constexpr value. */
        reg::ReadWrite(0x500038F8, REG_BITS_VALUE(0, 1, 0));

        /* Clear the tsec host1x syncpoint. */
        reg::Write(0x50003300, 0);
    }

}