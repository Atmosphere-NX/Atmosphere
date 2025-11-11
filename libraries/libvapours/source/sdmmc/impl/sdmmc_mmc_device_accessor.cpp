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
#if defined(ATMOSPHERE_IS_STRATOSPHERE)
#include <stratosphere.hpp>
#elif defined(ATMOSPHERE_IS_MESOSPHERE)
#include <mesosphere.hpp>
#elif defined(ATMOSPHERE_IS_EXOSPHERE)
#include <exosphere.hpp>
#else
#include <vapours.hpp>
#endif
#include "sdmmc_mmc_device_accessor.hpp"
#include "sdmmc_timer.hpp"

namespace ams::sdmmc::impl {

    #if defined(AMS_SDMMC_THREAD_SAFE)

        #define AMS_SDMMC_LOCK_MMC_DEVICE_MUTEX() std::scoped_lock lk(m_mmc_device.m_device_mutex)

    #else

        #define AMS_SDMMC_LOCK_MMC_DEVICE_MUTEX()

    #endif

    namespace {

        constexpr inline u32 OcrCardPowerUpStatus     = (1 << 31);

        constexpr inline u32 OcrAccessMode_Mask       = (3 << 29);
        constexpr inline u32 OcrAccessMode_SectorMode = (2 << 29);

        constexpr inline u8 ManufacturerId_Toshiba    = 0x11;

        enum DeviceType : u8 {
            DeviceType_HighSpeed26MHz              = (1u << 0),
            DeviceType_HighSpeed52MHz              = (1u << 1),
            DeviceType_HighSpeedDdr52MHz1_8VOr3_0V = (1u << 2),
            DeviceType_HighSpeedDdr52MHz1_2V       = (1u << 3),
            DeviceType_Hs200Sdr200MHz1_8V          = (1u << 4),
            DeviceType_Hs200Sdr200MHz1_2V          = (1u << 5),
            DeviceType_Hs400Sdr200MHz1_8V          = (1u << 6),
            DeviceType_Hs400Sdr200MHz1_2V          = (1u << 7),
        };

        constexpr bool IsToshibaMmc(const u8 *cid) {
            /* Check whether the CID's manufacturer id field is Toshiba. */
            AMS_ABORT_UNLESS(cid != nullptr);
            return cid[14] == ManufacturerId_Toshiba;
        }

        constexpr bool IsLessThanSpecification4(const u8 *csd) {
            const u8 spec_vers = ((csd[14] >> 2) & 0xF);
            return spec_vers < 4;
        }

        constexpr bool IsBkopAutoEnable(const u8 *ext_csd) {
            /* Check the AUTO_EN bit of BKOPS_EN. */
            return (ext_csd[163] & (1u << 1)) != 0;
        }

        constexpr u8 GetDeviceType(const u8 *ext_csd) {
            /* Get the DEVICE_TYPE register. */
            AMS_ABORT_UNLESS(ext_csd != nullptr);
            return ext_csd[196];
        }

        constexpr bool IsSupportedHs400(u8 device_type) {
            return (device_type & DeviceType_Hs400Sdr200MHz1_8V) != 0;
        }

        constexpr bool IsSupportedHs200(u8 device_type) {
            return (device_type & DeviceType_Hs200Sdr200MHz1_8V) != 0;
        }

        constexpr bool IsSupportedHighSpeed(u8 device_type) {
            return (device_type & DeviceType_HighSpeed52MHz) != 0;
        }

        constexpr u32 GetMemoryCapacityFromExtCsd(const u32 *ext_csd) {
            /* Get the SEC_COUNT register. */
            AMS_ABORT_UNLESS(ext_csd != nullptr);
            return ext_csd[212 / sizeof(u32)];
        }

        constexpr u32 GetBootPartitionMemoryCapacityFromExtCsd(const u8 *ext_csd) {
            /* Get the BOOT_SIZE_MULT register. */
            AMS_ABORT_UNLESS(ext_csd != nullptr);
            return ext_csd[226] * (128_KB / SectorSize);
        }

        constexpr Result GetCurrentSpeedModeFromExtCsd(SpeedMode *out, const u8 *ext_csd) {
            /* Get the HS_TIMING register. */
            AMS_ABORT_UNLESS(out     != nullptr);
            AMS_ABORT_UNLESS(ext_csd != nullptr);

            switch (ext_csd[185] & 0xF) {
                case 0:  *out = SpeedMode_MmcLegacySpeed; break;
                case 1:  *out = SpeedMode_MmcHighSpeed;   break;
                case 2:  *out = SpeedMode_MmcHs200;       break;
                case 3:  *out = SpeedMode_MmcHs400;       break;
                default: R_THROW(sdmmc::ResultUnexpectedMmcExtendedCsdValue());
            }

            R_SUCCEED();
        }

    }

    void MmcDevice::SetOcrAndHighCapacity(u32 ocr) {
        /* Set ocr. */
        BaseDevice::SetOcr(ocr);

        /* Set high capacity. */
        BaseDevice::SetHighCapacity((ocr & OcrAccessMode_Mask) == OcrAccessMode_SectorMode);
    }

    Result MmcDeviceAccessor::IssueCommandSendOpCond(u32 *out_ocr, BusPower bus_power) const {
        /* Get the command argument. */
        u32 arg = OcrAccessMode_SectorMode;
        switch (bus_power) {
            case BusPower_1_8V: arg |= 0x000080; break;
            case BusPower_3_3V: arg |= 0x03F800; break;
            AMS_UNREACHABLE_DEFAULT_CASE();
        }

        /* Issue the command. */
        constexpr ResponseType CommandResponseType = ResponseType_R3;
        Command command(CommandIndex_SendOpCond, arg, CommandResponseType, false);
        IHostController *hc = BaseDeviceAccessor::GetHostController();
        R_TRY(hc->IssueCommand(std::addressof(command)));

        /* Get the response. */
        hc->GetLastResponse(out_ocr, sizeof(*out_ocr), CommandResponseType);

        R_SUCCEED();
    }

    Result MmcDeviceAccessor::IssueCommandSetRelativeAddr() const {
        /* Get rca. */
        const u32 rca = m_mmc_device.GetRca();
        AMS_ABORT_UNLESS(rca > 0);

        /* Issue comamnd. */
        const u32 arg = rca << 16;
        R_TRY(BaseDeviceAccessor::IssueCommandAndCheckR1(CommandIndex_SetRelativeAddr, arg, false, DeviceState_Unknown));

        R_SUCCEED();
    }

    Result MmcDeviceAccessor::IssueCommandSwitch(CommandSwitch cs) const {
        /* Get the command argument. */
        const u32 arg = GetCommandSwitchArgument(cs);

        /* Issue the command. */
        R_TRY(BaseDeviceAccessor::IssueCommandAndCheckR1(CommandIndex_Switch, arg, true, DeviceState_Unknown));

        R_SUCCEED();
    }

    Result MmcDeviceAccessor::IssueCommandSendExtCsd(void *dst, size_t dst_size) const {
        /* Validate the output buffer. */
        AMS_ABORT_UNLESS(dst != nullptr);
        AMS_ABORT_UNLESS(dst_size >= MmcExtendedCsdSize);

        /* Issue the command. */
        constexpr ResponseType CommandResponseType = ResponseType_R1;
        Command command(CommandIndex_SendExtCsd, 0, CommandResponseType, false);
        TransferData xfer_data(dst, MmcExtendedCsdSize, 1, TransferDirection_ReadFromDevice);
        IHostController *hc = BaseDeviceAccessor::GetHostController();
        R_TRY(hc->IssueCommand(std::addressof(command), std::addressof(xfer_data)));

        /* Get the response. */
        u32 resp;
        hc->GetLastResponse(std::addressof(resp), sizeof(resp), CommandResponseType);
        R_TRY(m_mmc_device.CheckDeviceStatus(resp));

        R_SUCCEED();
    }

    Result MmcDeviceAccessor::IssueCommandEraseGroupStart(u32 sector_index) const {
        /* Get the command argument. */
        const u32 arg = m_mmc_device.IsHighCapacity() ? sector_index : sector_index * SectorSize;

        /* Issue the command. */
        R_TRY(BaseDeviceAccessor::IssueCommandAndCheckR1(CommandIndex_EraseGroupStart, arg, false, DeviceState_Unknown));

        R_SUCCEED();
    }

    Result MmcDeviceAccessor::IssueCommandEraseGroupEnd(u32 sector_index) const {
        /* Get the command argument. */
        const u32 arg = m_mmc_device.IsHighCapacity() ? sector_index : sector_index * SectorSize;

        /* Issue the command. */
        R_TRY(BaseDeviceAccessor::IssueCommandAndCheckR1(CommandIndex_EraseGroupEnd, arg, false, DeviceState_Tran));

        R_SUCCEED();
    }

    Result MmcDeviceAccessor::IssueCommandErase() const {
        /* Issue the command. */
        R_TRY(BaseDeviceAccessor::IssueCommandAndCheckR1(CommandIndex_Erase, 0, false, DeviceState_Tran));

        R_SUCCEED();
    }

    Result MmcDeviceAccessor::CancelToshibaMmcModel() {
        /* Special erase sequence done by Nintendo on Toshiba MMCs. */
        R_TRY(this->IssueCommandSwitch(CommandSwitch_SetBitsProductionStateAwarenessEnable));
        R_TRY(BaseDeviceAccessor::IssueCommandSendStatus());
        R_TRY(this->IssueCommandSwitch(CommandSwitch_ClearBitsAutoModeEnable));
        R_TRY(BaseDeviceAccessor::IssueCommandSendStatus());
        R_TRY(this->IssueCommandSwitch(CommandSwitch_WriteProductionStateAwarenessPreSolderingWrites));
        R_TRY(BaseDeviceAccessor::IssueCommandSendStatus());
        R_TRY(this->IssueCommandSwitch(CommandSwitch_WriteProductionStateAwarenessPreSolderingPostWrites));
        R_TRY(BaseDeviceAccessor::IssueCommandSendStatus());
        R_TRY(this->IssueCommandSwitch(CommandSwitch_WriteProductionStateAwarenessNormal));
        R_TRY(BaseDeviceAccessor::IssueCommandSendStatus());

        R_SUCCEED();
    }

    Result MmcDeviceAccessor::ChangeToReadyState(BusPower bus_power) {
        /* Be prepared to wait up to 1.5 seconds to change state. */
        ManualTimer timer(1500);
        while (true) {
            /* Get the ocr, and check if we're done. */
            u32 ocr;
            R_TRY(this->IssueCommandSendOpCond(std::addressof(ocr), bus_power));
            if ((ocr & OcrCardPowerUpStatus) != 0) {
                m_mmc_device.SetOcrAndHighCapacity(ocr);
                R_SUCCEED();
            }

            /* Check if we've timed out. */
            R_UNLESS(timer.Update(), sdmmc::ResultMmcInitializationSoftwareTimeout());

            /* Try again in 1ms. */
            WaitMicroSeconds(1000);
        }
    }

    Result MmcDeviceAccessor::ExtendBusWidth(BusWidth max_bw) {
        /* If the maximum bus width is 1bit, we can't extend. */
        R_SUCCEED_IF(max_bw == BusWidth_1Bit);

        /* Determine what bus width to switch to. */
        IHostController *hc = BaseDeviceAccessor::GetHostController();
        BusWidth target_bw = BusWidth_1Bit;
        CommandSwitch cs;
        if (max_bw == BusWidth_8Bit && hc->IsSupportedBusWidth(BusWidth_8Bit)) {
            target_bw = BusWidth_8Bit;
            cs        = CommandSwitch_WriteBusWidth8Bit;
        } else if ((max_bw == BusWidth_8Bit || max_bw == BusWidth_4Bit) && hc->IsSupportedBusWidth(BusWidth_4Bit)) {
            target_bw = BusWidth_4Bit;
            cs        = CommandSwitch_WriteBusWidth4Bit;
        } else {
            /* Target bus width is 1bit. */
            R_SUCCEED();
        }

        /* Set the bus width. */
        R_TRY(this->IssueCommandSwitch(cs));
        R_TRY(BaseDeviceAccessor::IssueCommandSendStatus());
        hc->SetBusWidth(target_bw);

        R_SUCCEED();
    }

    Result MmcDeviceAccessor::EnableBkopsAuto() {
        /* Issue the command. */
        R_TRY(this->IssueCommandSwitch(CommandSwitch_SetBitsBkopsEnAutoEn));
        R_TRY(BaseDeviceAccessor::IssueCommandSendStatus());

        R_SUCCEED();
    }

    Result MmcDeviceAccessor::ChangeToHighSpeed(bool check_before) {
        /* Issue high speed command. */
        R_TRY(this->IssueCommandSwitch(CommandSwitch_WriteHsTimingHighSpeed));

        /* If we should check status before setting mode, do so. */
        if (check_before) {
            R_TRY(BaseDeviceAccessor::IssueCommandSendStatus());
        }

        /* Set the host controller to high speed. */
        R_TRY(BaseDeviceAccessor::GetHostController()->SetSpeedMode(SpeedMode_MmcHighSpeed));

        /* If we should check status after setting mode, do so. */
        if (!check_before) {
            R_TRY(BaseDeviceAccessor::IssueCommandSendStatus());
        }

        R_SUCCEED();
    }

    Result MmcDeviceAccessor::ChangeToHs200() {
        /* Issue Hs200 command. */
        R_TRY(this->IssueCommandSwitch(CommandSwitch_WriteHsTimingHs200));

        /* Set the host controller to Hs200. */
        IHostController *hc = BaseDeviceAccessor::GetHostController();
        R_TRY(hc->SetSpeedMode(SpeedMode_MmcHs200));

        /* Perform tuning using command index 21. */
        R_TRY(hc->Tuning(SpeedMode_MmcHs200, 21));

        /* Check status. */
        R_TRY(BaseDeviceAccessor::IssueCommandSendStatus());

        R_SUCCEED();
    }

    Result MmcDeviceAccessor::ChangeToHs400() {
        /* Change first to Hs200. */
        R_TRY(this->ChangeToHs200());

        /* Save tuning status. */
        IHostController *hc = BaseDeviceAccessor::GetHostController();
        hc->SaveTuningStatusForHs400();

        /* Change to high speed. */
        R_TRY(this->ChangeToHighSpeed(false));

        /* Issue Hs400 command. */
        R_TRY(this->IssueCommandSwitch(CommandSwitch_WriteBusWidth8BitDdr));
        R_TRY(this->IssueCommandSwitch(CommandSwitch_WriteHsTimingHs400));

        /* Set the host controller to Hs400. */
        R_TRY(hc->SetSpeedMode(SpeedMode_MmcHs400));

        /* Check status. */
        R_TRY(BaseDeviceAccessor::IssueCommandSendStatus());

        R_SUCCEED();
    }

    Result MmcDeviceAccessor::ExtendBusSpeed(u8 device_type, SpeedMode max_sm) {
        /* We want to switch to the highest speed we can. */
        /* Check Hs400/Hs200 first. */
        IHostController *hc = BaseDeviceAccessor::GetHostController();
        if (hc->IsSupportedTuning() && hc->GetBusPower() == BusPower_1_8V) {
            if (hc->GetBusWidth() == BusWidth_8Bit && IsSupportedHs400(device_type) && max_sm == SpeedMode_MmcHs400) {
                R_RETURN(this->ChangeToHs400());
            } else if ((hc->GetBusWidth() == BusWidth_8Bit ||  hc->GetBusWidth() == BusWidth_4Bit) && IsSupportedHs200(device_type) && (max_sm == SpeedMode_MmcHs400 || max_sm == SpeedMode_MmcHs200)) {
                R_RETURN(this->ChangeToHs200());
            }
        }

        /* Check if we can switch to high speed. */
        if (IsSupportedHighSpeed(device_type)) {
            R_RETURN(this->ChangeToHighSpeed(true));
        }

        /* We can't, so stay at normal speeds. */
        R_SUCCEED();
    }

    Result MmcDeviceAccessor::StartupMmcDevice(BusWidth max_bw, SpeedMode max_sm, void *wb, size_t wb_size) {
        /* Start up at an appropriate bus power. */
        IHostController *hc = BaseDeviceAccessor::GetHostController();
        const BusPower bp   = hc->IsSupportedBusPower(BusPower_1_8V) ? BusPower_1_8V : BusPower_3_3V;
        R_TRY(hc->Startup(bp, BusWidth_1Bit, SpeedMode_MmcIdentification, false));

        /* Wait 1ms for configuration to take. */
        WaitMicroSeconds(1000);

        /* Wait an additional 74 clocks for configuration to take. */
        WaitClocks(74, hc->GetDeviceClockFrequencyKHz());

        /* Go to idle state. */
        R_TRY(BaseDeviceAccessor::IssueCommandGoIdleState());
        m_current_partition = MmcPartition_UserData;

        /* Go to ready state. */
        R_TRY(this->ChangeToReadyState(bp));

        /* Get the CID. */
        R_TRY(BaseDeviceAccessor::IssueCommandAllSendCid(wb, wb_size));
        m_mmc_device.SetCid(wb, wb_size);
        const bool is_toshiba = IsToshibaMmc(static_cast<const u8 *>(wb));

        /* Issue set relative addr. */
        R_TRY(this->IssueCommandSetRelativeAddr());

        /* Get the CSD. */
        R_TRY(BaseDeviceAccessor::IssueCommandSendCsd(wb, wb_size));
        m_mmc_device.SetCsd(wb, wb_size);
        const bool spec_under_4 = IsLessThanSpecification4(static_cast<const u8 *>(wb));

        /* Set the speed mode to legacy. */
        R_TRY(hc->SetSpeedMode(SpeedMode_MmcLegacySpeed));

        /* Issue select card command. */
        R_TRY(BaseDeviceAccessor::IssueCommandSelectCard());

        /* Set block length to sector size. */
        R_TRY(BaseDeviceAccessor::IssueCommandSetBlockLenToSectorSize());

        /* If the device SPEC_VERS is less than 4, extended csd/switch aren't supported. */
        if (spec_under_4) {
            R_TRY(m_mmc_device.SetLegacyMemoryCapacity());

            m_mmc_device.SetActive();
            R_SUCCEED();
        }

        /* Extend the bus width to the largest that we can. */
        R_TRY(this->ExtendBusWidth(max_bw));

        /* Get the extended csd. */
        R_TRY(this->IssueCommandSendExtCsd(wb, wb_size));
        AMS_ABORT_UNLESS(util::IsAligned(reinterpret_cast<uintptr_t>(wb), alignof(u32)));
        m_mmc_device.SetMemoryCapacity(GetMemoryCapacityFromExtCsd(static_cast<const u32 *>(wb)));

        /* If the mmc is manufactured by toshiba, try to enable bkops auto. */
        if (is_toshiba && !IsBkopAutoEnable(static_cast<const u8 *>(wb))) {
            /* NOTE: Nintendo does not check the result of this. */
            static_cast<void>(this->EnableBkopsAuto());
        }

        /* Extend the bus speed to as fast as we can. */
        const u8 device_type = GetDeviceType(static_cast<const u8 *>(wb));
        R_TRY(this->ExtendBusSpeed(device_type, max_sm));

        /* Enable power saving. */
        hc->SetPowerSaving(true);

        R_SUCCEED();
    }

    Result MmcDeviceAccessor::OnActivate() {
        /* Define the possible startup parameters. */
        constexpr const struct {
            BusWidth bus_width;
            SpeedMode speed_mode;
        } StartupParameters[] = {
            #if defined(AMS_SDMMC_ENABLE_MMC_HS400)
                { BusWidth_8Bit, SpeedMode_MmcHs400 },
            #else
                { BusWidth_8Bit, SpeedMode_MmcHighSpeed },
            #endif
            { BusWidth_8Bit, SpeedMode_MmcHighSpeed },
            { BusWidth_1Bit, SpeedMode_MmcHighSpeed },
        };

        /* Try to start up with each set of parameters. */
        Result result;
        for (int i = 0; i < static_cast<int>(util::size(StartupParameters)); ++i) {
            /* Alias the parameters. */
            const auto &params = StartupParameters[i];

            /* Set our max bus width/speed mode. */
            m_max_bus_width  = params.bus_width;
            m_max_speed_mode = params.speed_mode;

            /* Try to start up the device. */
            result = this->StartupMmcDevice(m_max_bus_width, m_max_speed_mode, m_work_buffer, m_work_buffer_size);
            if (R_SUCCEEDED(result)) {
                /* If we previously failed to start up the device, log the error correction. */
                if (i != 0) {
                    BaseDeviceAccessor::PushErrorLog(true, "S %d %d:0", m_max_bus_width, m_max_speed_mode);
                    BaseDeviceAccessor::IncrementNumActivationErrorCorrections();
                }

                R_SUCCEED();
            }

            /* Log that our startup failed. */
            BaseDeviceAccessor::PushErrorLog(false, "S %d %d:%X", m_max_bus_width, m_max_speed_mode, result.GetValue());

            /* Shut down the host controller before we try to start up again. */
            BaseDeviceAccessor::GetHostController()->Shutdown();
        }

        /* We failed to start up with all sets of parameters. */
        BaseDeviceAccessor::PushErrorTimeStamp();

        R_RETURN(result);
    }

    Result MmcDeviceAccessor::OnReadWrite(u32 sector_index, u32 num_sectors, void *buf, size_t buf_size, bool is_read) {
        /* Get the sector index alignment. */
        u32 sector_index_alignment = 0;
        if (!is_read) {
            constexpr u32 MmcWriteSectorAlignment = 16_KB / SectorSize;
            sector_index_alignment = MmcWriteSectorAlignment;
            AMS_ABORT_UNLESS(util::IsAligned(sector_index, MmcWriteSectorAlignment));
        }

        /* Do the read/write. */
        R_RETURN(BaseDeviceAccessor::ReadWriteMultiple(sector_index, num_sectors, sector_index_alignment, buf, buf_size, is_read));
    }

    Result MmcDeviceAccessor::ReStartup() {
        /* Shut down the host controller. */
        BaseDeviceAccessor::GetHostController()->Shutdown();

        /* Perform start up. */
        Result result = this->StartupMmcDevice(m_max_bus_width, m_max_speed_mode, m_work_buffer, m_work_buffer_size);
        if (R_FAILED(result)) {
            BaseDeviceAccessor::PushErrorLog(false, "S %d %d:%X", m_max_bus_width, m_max_speed_mode, result.GetValue());
            R_RETURN(result);
        }

        R_SUCCEED();
    }

    void MmcDeviceAccessor::Initialize() {
        /* Acquire exclusive access to the device. */
        AMS_SDMMC_LOCK_MMC_DEVICE_MUTEX();

        /* If we've already initialized, we don't need to do anything. */
        if (m_is_initialized) {
            return;
        }

        /* Set the base device to our mmc device. */
        BaseDeviceAccessor::SetDevice(std::addressof(m_mmc_device));

        /* Initialize. */
        BaseDeviceAccessor::GetHostController()->Initialize();
        m_is_initialized = true;
    }

    void MmcDeviceAccessor::Finalize() {
        /* Acquire exclusive access to the device. */
        AMS_SDMMC_LOCK_MMC_DEVICE_MUTEX();

        /* If we've already finalized, we don't need to do anything. */
        if (!m_is_initialized) {
            return;
        }
        m_is_initialized = false;

        /* Deactivate the device. */
        BaseDeviceAccessor::Deactivate();

        /* Finalize the host controller. */
        BaseDeviceAccessor::GetHostController()->Finalize();
    }

    Result MmcDeviceAccessor::GetSpeedMode(SpeedMode *out_speed_mode) const {
        /* Check that we can write to output. */
        AMS_ABORT_UNLESS(out_speed_mode != nullptr);

        /* Get the current speed mode from the ext csd. */
        R_TRY(GetMmcExtendedCsd(m_work_buffer, m_work_buffer_size));
        R_TRY(GetCurrentSpeedModeFromExtCsd(out_speed_mode, static_cast<const u8 *>(m_work_buffer)));

        R_SUCCEED();
    }

    void MmcDeviceAccessor::PutMmcToSleep() {
        /* Acquire exclusive access to the device. */
        AMS_SDMMC_LOCK_MMC_DEVICE_MUTEX();

        /* If the device isn't awake, we don't need to do anything. */
        if (!m_mmc_device.IsAwake()) {
            return;
        }

        /* Put the device to sleep. */
        m_mmc_device.PutToSleep();

        /* If necessary, put the host controller to sleep. */
        if (m_mmc_device.IsActive()) {
            BaseDeviceAccessor::GetHostController()->PutToSleep();
        }
    }

    void MmcDeviceAccessor::AwakenMmc() {
        /* Acquire exclusive access to the device. */
        AMS_SDMMC_LOCK_MMC_DEVICE_MUTEX();

        /* If the device is awake, we don't need to do anything. */
        if (m_mmc_device.IsAwake()) {
            return;
        }

        /* Wake the host controller, if we need to.*/
        if (m_mmc_device.IsActive()) {
            const Result result = BaseDeviceAccessor::GetHostController()->Awaken();
            if (R_FAILED(result)) {
                BaseDeviceAccessor::PushErrorLog(true, "A:%X", result.GetValue());
            }
        }

        /* Wake the device. */
        m_mmc_device.Awaken();
    }

    Result MmcDeviceAccessor::SelectMmcPartition(MmcPartition part) {
        /* Acquire exclusive access to the device. */
        AMS_SDMMC_LOCK_MMC_DEVICE_MUTEX();

        /* Check that we can access the device. */
        R_TRY(m_mmc_device.CheckAccessible());

        /* Determine the appropriate SWITCH subcommand. */
        CommandSwitch cs;
        switch (part) {
            case MmcPartition_UserData:       cs = CommandSwitch_WritePartitionAccessDefault;          break;
            case MmcPartition_BootPartition1: cs = CommandSwitch_WritePartitionAccessRwBootPartition1; break;
            case MmcPartition_BootPartition2: cs = CommandSwitch_WritePartitionAccessRwBootPartition2; break;
            AMS_UNREACHABLE_DEFAULT_CASE();
        }

        /* Change partition. */
        m_current_partition = MmcPartition_Unknown;
        {
            R_TRY(this->IssueCommandSwitch(cs));
            R_TRY(BaseDeviceAccessor::IssueCommandSendStatus());
        }
        m_current_partition = part;

        R_SUCCEED();
    }

    Result MmcDeviceAccessor::EraseMmc() {
        /* Acquire exclusive access to the device. */
        AMS_SDMMC_LOCK_MMC_DEVICE_MUTEX();

        /* Check that we can access the device. */
        R_TRY(m_mmc_device.CheckAccessible());

        /* Get the partition capacity. */
        u32 part_capacity;
        switch (m_current_partition) {
            case MmcPartition_UserData:
                part_capacity = m_mmc_device.GetMemoryCapacity();
                break;
            case MmcPartition_BootPartition1:
            case MmcPartition_BootPartition2:
                R_TRY(this->GetMmcBootPartitionCapacity(std::addressof(part_capacity)));
                break;
            AMS_UNREACHABLE_DEFAULT_CASE();
        }

        /* Begin the erase. */
        R_TRY(this->IssueCommandEraseGroupStart(0));
        R_TRY(this->IssueCommandEraseGroupEnd(part_capacity - 1));

        /* Issue the erase command, allowing 30 seconds for it to complete. */
        ManualTimer timer(30000);
        Result result = this->IssueCommandErase();
        R_TRY_CATCH(result) {
            R_CATCH(sdmmc::ResultDataTimeoutError)               { /* Data timeout error is acceptable. */ }
            R_CATCH(sdmmc::ResultCommandCompleteSoftwareTimeout) { /* Command complete software timeout error is acceptable. */ }
            R_CATCH(sdmmc::ResultBusySoftwareTimeout)            { /* Busy software timeout error is acceptable. */ }
        } R_END_TRY_CATCH;

        /* Wait for the erase to finish. */
        while (true) {
            /* Check if we're done. */
            result = BaseDeviceAccessor::IssueCommandSendStatus();
            if (R_SUCCEEDED(result)) {
                break;
            }

            /* Otherwise, check if we should reject the error. */
            if (!sdmmc::ResultUnexpectedDeviceState::Includes(result)) {
                R_RETURN(result);
            }

            /* Check if timeout has been exceeded. */
            R_UNLESS(timer.Update(), sdmmc::ResultMmcEraseSoftwareTimeout());
        }

        /* If the partition is user data, check if we need to perform toshiba-specific erase. */
        if (m_current_partition == MmcPartition_UserData) {
            u8 cid[DeviceCidSize];
            m_mmc_device.GetCid(cid, sizeof(cid));
            if (IsToshibaMmc(cid)) {
                /* NOTE: Nintendo does not check the result of this operation. */
                static_cast<void>(this->CancelToshibaMmcModel());
            }
        }

        R_SUCCEED();
    }

    Result MmcDeviceAccessor::GetMmcBootPartitionCapacity(u32 *out_num_sectors) const {
        /* Get the capacity from the extended csd. */
        AMS_ABORT_UNLESS(out_num_sectors != nullptr);
        R_TRY(this->GetMmcExtendedCsd(m_work_buffer, m_work_buffer_size));

        *out_num_sectors = GetBootPartitionMemoryCapacityFromExtCsd(static_cast<const u8 *>(m_work_buffer));
        R_SUCCEED();
    }

    Result MmcDeviceAccessor::GetMmcExtendedCsd(void *dst, size_t dst_size) const {
        /* Acquire exclusive access to the device. */
        AMS_SDMMC_LOCK_MMC_DEVICE_MUTEX();

        /* Check that we can access the device. */
        R_TRY(m_mmc_device.CheckAccessible());

        /* Get the csd. */
        u8 csd[DeviceCsdSize];
        m_mmc_device.GetCsd(csd, sizeof(csd));

        /* Check that the card supports ext csd. */
        R_UNLESS(!IsLessThanSpecification4(csd), sdmmc::ResultMmcNotSupportExtendedCsd());

        /* Get the ext csd. */
        R_TRY(this->IssueCommandSendExtCsd(dst, dst_size));

        R_SUCCEED();
    }

}
