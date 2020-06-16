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
#include <exosphere.hpp>
#include "sc7fw_util.hpp"
#include "sc7fw_dram.hpp"

namespace ams::sc7fw {

    namespace {

        constexpr inline const uintptr_t PMC = secmon::MemoryRegionPhysicalDevicePmc.GetAddress();

        void UpdateEmcTiming() {
            /* Enable timing update. */
            reg::Write(EMC_ADDRESS(EMC_TIMING_CONTROL), EMC_REG_BITS_ENUM(TIMING_CONTROL_TIMING_UPDATE, ENABLED));

            /* Wait for the timing update to complete. */
            while (!reg::HasValue(EMC_ADDRESS(EMC_EMC_STATUS), EMC_REG_BITS_ENUM(EMC_STATUS_TIMING_UPDATE_STALLED, DONE))) {
                /* ... */
            }
        }

        void RequestAllPadsPowerDown(uintptr_t addr, uintptr_t expected) {
            constexpr u32 DpdAllRequestValue = reg::Encode(PMC_REG_BITS_ENUM(IO_DPD_REQ_CODE, DPD_ON)) | 0x0FFFFFFF;
            const auto RequestAddress = addr;
            const auto StatusAddress  = addr + 4;

            /* Request all pads enter power down. */
            reg::Write(PMC + RequestAddress, DpdAllRequestValue);

            /* Wait until the status reflects our expectation (and all pads are shut down). */
            while (reg::Read(PMC + StatusAddress) != expected) { /* ... */ }

            /* Wait a little while to allow the power down status to propagate. */
            SpinLoop(0x20);
        };

    }

    void SaveEmcFsp() {
        /* We require that the RAM is LPDDR4. */
        AMS_ABORT_UNLESS(reg::HasValue(EMC_ADDRESS(EMC_FBIO_CFG5), EMC_REG_BITS_ENUM(FBIO_CFG5_DRAM_TYPE, LPDDR4)));

        /* Read the frequency set points from MRW3. */
        constexpr u32 FspShift = 6;
        constexpr u32 FspBits  = 2;
        constexpr u32 FspMask  = ((1u << FspBits) - 1) << FspShift;
        static_assert(FspMask == 0x000000C0);
        const u32 fsp = (reg::Read(EMC_ADDRESS(EMC_MRW3)) & FspMask) >> FspShift;

        /* Write the fsp to PMC_SCRATCH18, where it will be restored to MRW3 by brom. */
        reg::ReadWrite(PMC + APBDEV_PMC_SCRATCH18, REG_BITS_VALUE(FspShift, FspBits, fsp));

        /* Write the fsp twice to PMC_SCRATCH12, where it will be restored to MRW12 by brom. */
        reg::ReadWrite(PMC + APBDEV_PMC_SCRATCH12, REG_BITS_VALUE(FspShift, FspBits, fsp), REG_BITS_VALUE(FspShift + 8, FspBits, fsp));

        /* Write the fsp twice to PMC_SCRATCH13, where it will be restored to MRW13 by brom. */
        reg::ReadWrite(PMC + APBDEV_PMC_SCRATCH13, REG_BITS_VALUE(FspShift, FspBits, fsp), REG_BITS_VALUE(FspShift + 8, FspBits, fsp));
    }

    void EnableSdramSelfRefresh() {
        /* We require that the RAM is dual-channel. */
        AMS_ABORT_UNLESS(reg::HasValue(EMC_ADDRESS(EMC_FBIO_CFG7), EMC_REG_BITS_ENUM(FBIO_CFG7_CH1_ENABLE, ENABLE)));

        /* Disable RAM's ability to dynamically self-refresh, and to opportunistically perform powerdown. */
        reg::Write(EMC_ADDRESS(EMC_CFG), EMC_REG_BITS_ENUM(CFG_DYN_SELF_REF,     DISABLED),
                                         EMC_REG_BITS_ENUM(CFG_DRAM_ACPD,    NO_POWERDOWN));

        /* Update the EMC timing. */
        UpdateEmcTiming();

        /* Wait five microseconds. */
        util::WaitMicroSeconds(5);

        /* Disable ZQ calibration. */
        reg::Write(EMC_ADDRESS(EMC_ZCAL_INTERVAL), 0);

        /* Disable automatic calibration. */
        reg::Write(EMC_ADDRESS(EMC_AUTO_CAL_CONFIG), EMC_REG_BITS_ENUM(AUTO_CAL_CONFIG_AUTO_CAL_MEASURE_STALL,  ENABLE),
                                                     EMC_REG_BITS_ENUM(AUTO_CAL_CONFIG_AUTO_CAL_UPDATE_STALL,   ENABLE),
                                                     EMC_REG_BITS_ENUM(AUTO_CAL_CONFIG_AUTO_CAL_START,         DISABLE));

        /* Get whether digital delay locked loops are enabled. */
        const bool has_dll = reg::HasValue(EMC_ADDRESS(EMC_CFG_DIG_DLL), EMC_REG_BITS_ENUM(CFG_DIG_DLL_CFG_DLL_EN,  ENABLED));
        if (has_dll) {
            /* If they are, disable them. */
            reg::ReadWrite(EMC_ADDRESS(EMC_CFG_DIG_DLL), EMC_REG_BITS_ENUM(CFG_DIG_DLL_CFG_DLL_EN,  DISABLED));
        }

        /* Update the EMC timing. */
        UpdateEmcTiming();

        /* If dll was enabled, wait until both EMC0 and EMC1 have dll disabled. */
        if (has_dll) {
            while (!reg::HasValue(EMC0_ADDRESS(EMC_CFG_DIG_DLL), EMC_REG_BITS_ENUM(CFG_DIG_DLL_CFG_DLL_EN, DISABLED))) { /* ... */ }
            while (!reg::HasValue(EMC1_ADDRESS(EMC_CFG_DIG_DLL), EMC_REG_BITS_ENUM(CFG_DIG_DLL_CFG_DLL_EN, DISABLED))) { /* ... */ }
        }

        /* Stall all reads and writes. */
        reg::Write(EMC_ADDRESS(EMC_REQ_CTRL), EMC_REG_BITS_VALUE(REQ_CTRL_STALL_ALL_READS,  1),
                                              EMC_REG_BITS_VALUE(REQ_CTRL_STALL_ALL_WRITES, 1));

        /* Wait until both EMC0 and EMC1 have no outstanding transactions. */
        while (!reg::HasValue(EMC0_ADDRESS(EMC_EMC_STATUS), EMC_REG_BITS_ENUM(EMC_STATUS_NO_OUTSTANDING_TRANSACTIONS, COMPLETED))) { /* ... */ }
        while (!reg::HasValue(EMC1_ADDRESS(EMC_EMC_STATUS), EMC_REG_BITS_ENUM(EMC_STATUS_NO_OUTSTANDING_TRANSACTIONS, COMPLETED))) { /* ... */ }

        /* Enable self-refresh. */
        reg::Write(EMC_ADDRESS(EMC_SELF_REF), EMC_REG_BITS_ENUM(SELF_REF_SREF_DEV_SELECTN,    BOTH),
                                              EMC_REG_BITS_ENUM(SELF_REF_SELF_REF_CMD,     ENABLED));

        /* Wait until both EMC and EMC1 are in self-refresh. */
        const auto desired = reg::HasValue(EMC_ADDRESS(EMC_ADR_CFG), EMC_REG_BITS_ENUM(ADR_CFG_EMEM_NUMDEV, N2)) ? EMC_REG_BITS_ENUM(EMC_STATUS_DRAM_IN_SELF_REFRESH,      BOTH_ENABLED)
                                                                                                                 : EMC_REG_BITS_ENUM(EMC_STATUS_DRAM_DEV0_IN_SELF_REFRESH,      ENABLED);

        /* NOTE: Nintendo's sc7 entry firmware has a bug here. */
        /* Instead of waiting for both EMCs to report self-refresh, they just read the EMC_STATUS for each EMC. */
        /* This is incorrect, per documentation. */
        while (!reg::HasValue(EMC0_ADDRESS(EMC_EMC_STATUS), desired)) { /* ... */ }
        while (!reg::HasValue(EMC1_ADDRESS(EMC_EMC_STATUS), desired)) { /* ... */ }
    }

    void EnableEmcAllSegmentsRefresh() {
        constexpr int MR17_PASR_Segment = 17;

        /* Write zeros to MR17_PASR_Segment to enable refresh for all segments for dev0. */
        reg::Write(EMC_ADDRESS(EMC_MRW), EMC_REG_BITS_ENUM (MRW_DEV_SELECTN,              DEV0),
                                         EMC_REG_BITS_ENUM (MRW_CNT,                      EXT1),
                                         EMC_REG_BITS_VALUE(MRW_MA,          MR17_PASR_Segment),
                                         EMC_REG_BITS_VALUE(MRW_OP,                          0));

        /* If dev1 exists, do the same for dev1. */
        if (reg::HasValue(EMC_ADDRESS(EMC_ADR_CFG), EMC_REG_BITS_ENUM(ADR_CFG_EMEM_NUMDEV, N2))) {
            reg::Write(EMC_ADDRESS(EMC_MRW), EMC_REG_BITS_ENUM (MRW_DEV_SELECTN,              DEV1),
                                             EMC_REG_BITS_ENUM (MRW_CNT,                      EXT1),
                                             EMC_REG_BITS_VALUE(MRW_MA,          MR17_PASR_Segment),
                                             EMC_REG_BITS_VALUE(MRW_OP,                          0));
        }
    }

    void EnableDdrDeepPowerDown() {
        /* Read and decode the parameters Nintendo stores in EMC_PMC_SCRATCH3. */
        const u32 scratch3   = reg::Read(EMC_ADDRESS(EMC_PMC_SCRATCH3));
        const bool weak_bias = (scratch3 & reg::EncodeMask(EMC_REG_BITS_MASK(PMC_SCRATCH3_WEAK_BIAS))) == reg::EncodeValue(EMC_REG_BITS_ENUM(PMC_SCRATCH3_WEAK_BIAS, ENABLED));
        const u32 ddr_cntrl  = (scratch3 & reg::EncodeMask(EMC_REG_BITS_MASK(PMC_SCRATCH3_DDR_CNTRL)));

        /* Write the decoded value to PMC_DDR_CNTRL. */
        reg::Write(PMC + APBDEV_PMC_DDR_CNTRL, ddr_cntrl);

        /* If weak bias is enabled, set all VTT_E_WB bits in APBDEV_PMC_WEAK_BIAS. */
        if (weak_bias) {
            constexpr u32 WeakBiasVttEWbAll = 0x7FFF0000;
            reg::Write(PMC + APBDEV_PMC_WEAK_BIAS, WeakBiasVttEWbAll);
        }

        /* Request that DPD3 pads power down. */
        constexpr u32 EristaDpd3Mask = 0x0FFFFFFF;
        constexpr u32 MarikoDpd3Mask = 0x0FFF9FFF;
        if (fuse::GetSocType() == fuse::SocType_Erista) {
            RequestAllPadsPowerDown(APBDEV_PMC_IO_DPD3_REQ, EristaDpd3Mask);
        } else {
            RequestAllPadsPowerDown(APBDEV_PMC_IO_DPD3_REQ, MarikoDpd3Mask);
        }

        /* Request that DPD4 pads power down. */
        constexpr u32 Dpd4Mask = 0x0FFF1FFF;
        RequestAllPadsPowerDown(APBDEV_PMC_IO_DPD4_REQ, Dpd4Mask);
    }

}
