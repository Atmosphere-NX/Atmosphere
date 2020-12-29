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
#if defined(ATMOSPHERE_IS_STRATOSPHERE)
#include <stratosphere.hpp>
#elif defined(ATMOSPHERE_IS_MESOSPHERE)
#include <mesosphere.hpp>
#elif defined(ATMOSPHERE_IS_EXOSPHERE)
#include <exosphere.hpp>
#else
#include <vapours.hpp>
#endif
#include "sdmmc_sd_card_device_accessor.hpp"
#include "sdmmc_timer.hpp"

namespace ams::sdmmc::impl {

    #if defined(AMS_SDMMC_THREAD_SAFE)

        #define AMS_SDMMC_LOCK_SD_CARD_DEVICE_MUTEX() std::scoped_lock lk(this->sd_card_device.device_mutex)

    #else

        #define AMS_SDMMC_LOCK_SD_CARD_DEVICE_MUTEX()

    #endif

    #if defined(AMS_SDMMC_USE_SD_CARD_DETECTOR)

        #define AMS_SDMMC_CHECK_SD_CARD_REMOVED() R_UNLESS(!this->sd_card_device.IsRemoved(), sdmmc::ResultDeviceRemoved())

    #else

        #define AMS_SDMMC_CHECK_SD_CARD_REMOVED()

    #endif

    namespace {

        constexpr inline u32 OcrCardPowerUpStatus       = (1 << 31);
        constexpr inline u32 OcrCardCapacityStatus      = (1 << 30);
        constexpr inline u32 OcrSwitchingTo1_8VAccepted = (1 << 24);

        constexpr bool IsLessThanSpecification1_1(const u8 *scr) {
            AMS_ABORT_UNLESS(scr != nullptr);

            const u8 sd_spec = scr[0] & 0xF;
            return sd_spec < 1;
        }

        constexpr u32 GetSendOpCmdArgument(bool spec_under_2, bool uhs_i_supported) {
            const u32 hcs  = !spec_under_2 ? (1u << 30) : (0u << 30);
            const u32 xpc  = !spec_under_2 ? (1u << 28) : (0u << 28);
            const u32 s18r = (!spec_under_2 && uhs_i_supported) ? (1u << 24) : (0u << 24);
            return hcs | xpc | s18r | 0x00100000u;
        }

        constexpr bool IsLessThanCsdVersion2(const u8 *csd) {
            AMS_ABORT_UNLESS(csd != nullptr);

            /* Check whether CSD_STRUCTURE is 0. */
            return ((csd[14] & 0xC0) >> 6) == 0;
        }

        constexpr u32 GetMemoryCapacityFromCsd(const u16 *csd) {
            AMS_ABORT_UNLESS(csd != nullptr);

            /* Get CSIZE, convert appropriately. */
            const u32 csize = (static_cast<u32>(csd[3] & 0x3FFF) << 8) | (static_cast<u32>(csd[2] & 0xFF00) >> 8);
            return (1 + csize) << 10;
        }

        constexpr u8 GetSdBusWidths(const u8 *scr) {
            AMS_ABORT_UNLESS(scr != nullptr);
            return scr[1] & 0xF;
        }

        constexpr bool IsSupportedBusWidth4Bit(u8 sd_bw) {
            return (sd_bw & 0x4) != 0;
        }

        constexpr bool IsSupportedAccessMode(const u8 *status, SwitchFunctionAccessMode access_mode) {
            AMS_ABORT_UNLESS(status != nullptr);

            return (status[13] & (1u << access_mode)) != 0;
        }

        constexpr u8 GetAccessModeFromFunctionSelection(const u8 *status) {
            AMS_ABORT_UNLESS(status != nullptr);
            return (status[16] & 0xF);
        }

        constexpr bool IsAccessModeInFunctionSelection(const u8 *status, SwitchFunctionAccessMode mode) {
            return GetAccessModeFromFunctionSelection(status) == static_cast<u8>(mode);
        }

        constexpr u16 GetMaximumCurrentConsumption(const u8 *status) {
            AMS_ABORT_UNLESS(status != nullptr);

            return (static_cast<u16>(status[0]) << 8) |
                   (static_cast<u16>(status[1]) << 0);
        }

        constexpr u32 GetSizeOfProtectedArea(const u8 *sd_status) {
            return (static_cast<u32>(sd_status[4]) << 24) |
                   (static_cast<u32>(sd_status[5]) << 16) |
                   (static_cast<u32>(sd_status[6]) <<  8) |
                   (static_cast<u32>(sd_status[7]) <<  0);
        }

        Result GetCurrentSpeedMode(SpeedMode *out_sm, const u8 *status, bool is_uhs_i) {
            AMS_ABORT_UNLESS(out_sm != nullptr);

            /* Get the access mode. */
            switch (static_cast<SwitchFunctionAccessMode>(GetAccessModeFromFunctionSelection(status))) {
                case SwitchFunctionAccessMode_Default:
                    if (is_uhs_i) {
                        *out_sm = SpeedMode_SdCardSdr12;
                    } else {
                        *out_sm = SpeedMode_SdCardDefaultSpeed;
                    }
                    break;
                case SwitchFunctionAccessMode_HighSpeed:
                    if (is_uhs_i) {
                        *out_sm = SpeedMode_SdCardSdr25;
                    } else {
                        *out_sm = SpeedMode_SdCardHighSpeed;
                    }
                    break;
                case SwitchFunctionAccessMode_Sdr50:
                    *out_sm = SpeedMode_SdCardSdr50;
                    break;
                case SwitchFunctionAccessMode_Sdr104:
                    *out_sm = SpeedMode_SdCardSdr104;
                    break;
                case SwitchFunctionAccessMode_Ddr50:
                    *out_sm = SpeedMode_SdCardDdr50;
                    break;
                default:
                    return sdmmc::ResultUnexpectedSdCardSwitchFunctionStatus();
            }

            return ResultSuccess();
        }

    }

    void SdCardDevice::SetOcrAndHighCapacity(u32 ocr) {
        /* Set ocr. */
        BaseDevice::SetOcr(ocr);

        /* Set high capacity. */
        BaseDevice::SetHighCapacity((ocr & OcrCardCapacityStatus) != 0);
    }

    #if defined(AMS_SDMMC_USE_SD_CARD_DETECTOR)
    void SdCardDeviceAccessor::RemovedCallback() {
        /* Signal that the device was removed. */
        this->sd_card_device.SignalRemovedEvent();

        /* Acquire exclusive access to the device. */
        AMS_SDMMC_LOCK_SD_CARD_DEVICE_MUTEX();

        /* Shut down. */
        BaseDeviceAccessor::GetHostController()->Shutdown();
    }
    #endif

    Result SdCardDeviceAccessor::IssueCommandSendRelativeAddr(u16 *out_rca) const {
        /* Issue the command. */
        constexpr ResponseType CommandResponseType = ResponseType_R6;
        Command command(CommandIndex_SendRelativeAddr, 0, CommandResponseType, false);
        IHostController *hc = BaseDeviceAccessor::GetHostController();
        R_TRY(hc->IssueCommand(std::addressof(command)));

        /* Get the response. */
        u32 resp;
        hc->GetLastResponse(std::addressof(resp), sizeof(resp), CommandResponseType);

        /* Set the output rca. */
        AMS_ABORT_UNLESS(out_rca != nullptr);
        *out_rca = static_cast<u16>(resp >> 16);

        return ResultSuccess();
    }

    Result SdCardDeviceAccessor::IssueCommandSendIfCond() const {
        /* Get the argument. */
        constexpr u32 SendIfCommandArgument     = 0x01AAu;
        constexpr u32 SendIfCommandArgumentMask = 0x0FFFu;

        /* Issue the command. */
        constexpr ResponseType CommandResponseType = ResponseType_R7;
        Command command(CommandIndex_SendIfCond, SendIfCommandArgument, CommandResponseType, false);
        IHostController *hc = BaseDeviceAccessor::GetHostController();
        R_TRY(hc->IssueCommand(std::addressof(command)));

        /* Get the response. */
        u32 resp;
        hc->GetLastResponse(std::addressof(resp), sizeof(resp), CommandResponseType);

        /* Verify that our argument was returned to us. */
        R_UNLESS((resp & SendIfCommandArgumentMask) == (SendIfCommandArgument & SendIfCommandArgumentMask), sdmmc::ResultSdCardValidationError());

        return ResultSuccess();
    }

    Result SdCardDeviceAccessor::IssueCommandCheckSupportedFunction(void *dst, size_t dst_size) const {
        /* Validate the output buffer. */
        AMS_ABORT_UNLESS(dst != nullptr);
        AMS_ABORT_UNLESS(dst_size >= SdCardSwitchFunctionStatusSize);

        /* Get the argument. */
        constexpr u32 CheckSupportedFunctionArgument = 0x00FFFFFF;

        /* Issue the command. */
        constexpr ResponseType CommandResponseType = ResponseType_R1;
        Command command(CommandIndex_Switch, CheckSupportedFunctionArgument, CommandResponseType, false);
        TransferData xfer_data(dst, SdCardSwitchFunctionStatusSize, 1, TransferDirection_ReadFromDevice);
        IHostController *hc = BaseDeviceAccessor::GetHostController();
        R_TRY(hc->IssueCommand(std::addressof(command), std::addressof(xfer_data)));

        /* Get the response. */
        u32 resp;
        hc->GetLastResponse(std::addressof(resp), sizeof(resp), CommandResponseType);
        R_TRY(this->sd_card_device.CheckDeviceStatus(resp));

        return ResultSuccess();
    }

    Result SdCardDeviceAccessor::IssueCommandSwitchAccessMode(void *dst, size_t dst_size, bool set_function, SwitchFunctionAccessMode access_mode) const {
        /* Validate the output buffer. */
        AMS_ABORT_UNLESS(dst != nullptr);
        AMS_ABORT_UNLESS(dst_size >= SdCardSwitchFunctionStatusSize);

        /* Get the argument. */
        const u32 arg = (set_function ? (1u << 31) : (0u << 31)) | 0x00FFFFF0 | static_cast<u32>(access_mode);

        /* Issue the command. */
        constexpr ResponseType CommandResponseType = ResponseType_R1;
        Command command(CommandIndex_Switch, arg, CommandResponseType, false);
        TransferData xfer_data(dst, SdCardSwitchFunctionStatusSize, 1, TransferDirection_ReadFromDevice);
        IHostController *hc = BaseDeviceAccessor::GetHostController();
        R_TRY(hc->IssueCommand(std::addressof(command), std::addressof(xfer_data)));

        /* Get the response. */
        u32 resp;
        hc->GetLastResponse(std::addressof(resp), sizeof(resp), CommandResponseType);
        R_TRY(this->sd_card_device.CheckDeviceStatus(resp));

        return ResultSuccess();
    }

    Result SdCardDeviceAccessor::IssueCommandVoltageSwitch() const {
        /* Issue the command. */
        R_TRY(BaseDeviceAccessor::IssueCommandAndCheckR1(CommandIndex_VoltageSwitch, 0, false, DeviceState_Ready));
        return ResultSuccess();
    }

    Result SdCardDeviceAccessor::IssueCommandAppCmd(DeviceState expected_state, u32 ignore_mask) const {
        /* Get arg. */
        const u32 arg = static_cast<u32>(this->sd_card_device.GetRca()) << 16;

        /* Issue the command. */
        constexpr ResponseType CommandResponseType = ResponseType_R1;
        Command command(CommandIndex_AppCmd, arg, CommandResponseType, false);
        IHostController *hc = BaseDeviceAccessor::GetHostController();
        R_TRY(hc->IssueCommand(std::addressof(command)));

        /* Get the response. */
        u32 resp;
        hc->GetLastResponse(std::addressof(resp), sizeof(resp), CommandResponseType);

        /* Mask out the ignored status bits. */
        if (ignore_mask != 0) {
            resp &= ~ignore_mask;
        }

        /* Check the device status. */
        R_TRY(this->sd_card_device.CheckDeviceStatus(resp));

        /* Check the app command bit. */
        R_UNLESS((resp & DeviceStatus_AppCmd) != 0, sdmmc::ResultUnexpectedSdCardAcmdDisabled());

        /* Check the device state. */
        if (expected_state != DeviceState_Unknown) {
            R_UNLESS(this->sd_card_device.GetDeviceState(resp) == expected_state, sdmmc::ResultUnexpectedDeviceState());
        }

        return ResultSuccess();
    }

    Result SdCardDeviceAccessor::IssueCommandSetBusWidth4Bit() const {
        /* Issue the application command. */
        constexpr u32 Arg = 0x2;
        R_TRY(BaseDeviceAccessor::IssueCommandAndCheckR1(SdApplicationCommandIndex_SetBusWidth, Arg, false, DeviceState_Tran));
        return ResultSuccess();
    }

    Result SdCardDeviceAccessor::IssueCommandSdStatus(void *dst, size_t dst_size) const {
        /* Validate the output buffer. */
        AMS_ABORT_UNLESS(dst != nullptr);
        AMS_ABORT_UNLESS(dst_size >= SdCardSdStatusSize);

        /* Issue the application command. */
        constexpr ResponseType CommandResponseType = ResponseType_R1;
        Command command(SdApplicationCommandIndex_SdStatus, 0, CommandResponseType, false);
        TransferData xfer_data(dst, SdCardSdStatusSize, 1, TransferDirection_ReadFromDevice);
        IHostController *hc = BaseDeviceAccessor::GetHostController();
        R_TRY(hc->IssueCommand(std::addressof(command), std::addressof(xfer_data)));

        /* Get the response. */
        u32 resp;
        hc->GetLastResponse(std::addressof(resp), sizeof(resp), CommandResponseType);
        R_TRY(this->sd_card_device.CheckDeviceStatus(resp));

        return ResultSuccess();
    }

    Result SdCardDeviceAccessor::IssueCommandSendOpCond(u32 *out_ocr, bool spec_under_2, bool uhs_i_supported) const {
        /* Get the argument. */
        const u32 arg = GetSendOpCmdArgument(spec_under_2, uhs_i_supported);

        /* Issue the application command. */
        constexpr ResponseType CommandResponseType = ResponseType_R3;
        Command command(SdApplicationCommandIndex_SdSendOpCond, arg, CommandResponseType, false);
        IHostController *hc = BaseDeviceAccessor::GetHostController();
        R_TRY(hc->IssueCommand(std::addressof(command)));

        /* Get the response. */
        hc->GetLastResponse(out_ocr, sizeof(u32), CommandResponseType);

        return ResultSuccess();
    }

    Result SdCardDeviceAccessor::IssueCommandClearCardDetect() const {
        /* Issue the application command. */
        R_TRY(BaseDeviceAccessor::IssueCommandAndCheckR1(SdApplicationCommandIndex_SetClearCardDetect, 0, false, DeviceState_Tran));
        return ResultSuccess();
    }

    Result SdCardDeviceAccessor::IssueCommandSendScr(void *dst, size_t dst_size) const {
        /* Validate the output buffer. */
        AMS_ABORT_UNLESS(dst != nullptr);
        AMS_ABORT_UNLESS(dst_size >= SdCardScrSize);

        /* Issue the application command. */
        constexpr ResponseType CommandResponseType = ResponseType_R1;
        Command command(SdApplicationCommandIndex_SendScr, 0, CommandResponseType, false);
        TransferData xfer_data(dst, SdCardScrSize, 1, TransferDirection_ReadFromDevice);
        IHostController *hc = BaseDeviceAccessor::GetHostController();
        R_TRY(hc->IssueCommand(std::addressof(command), std::addressof(xfer_data)));

        /* Get the response. */
        u32 resp;
        hc->GetLastResponse(std::addressof(resp), sizeof(resp), CommandResponseType);
        R_TRY(this->sd_card_device.CheckDeviceStatus(resp));

        return ResultSuccess();
    }

    Result SdCardDeviceAccessor::EnterUhsIMode() {
        /* Send voltage switch command. */
        R_TRY(this->IssueCommandVoltageSwitch());

        /* Switch to sdr12. */
        R_TRY(BaseDeviceAccessor::GetHostController()->SwitchToSdr12());

        return ResultSuccess();
    }

    Result SdCardDeviceAccessor::ChangeToReadyState(bool spec_under_2, bool uhs_i_supported) {
        /* Decide on an ignore mask. */
        u32 ignore_mask = spec_under_2 ? static_cast<u32>(DeviceStatus_IllegalCommand) : 0u;

        /* Be prepared to wait up to 3.0 seconds to change state. */
        ManualTimer timer(3000);
        while (true) {
            /* We want to get ocr, which requires our sending an application command. */
            R_TRY(this->IssueCommandAppCmd(DeviceState_Unknown, ignore_mask));
            ignore_mask = 0;

            /* Get the ocr, and check if we're done. */
            u32 ocr;
            R_TRY(this->IssueCommandSendOpCond(std::addressof(ocr), spec_under_2, uhs_i_supported));

            if ((ocr & OcrCardPowerUpStatus) != 0) {
                this->sd_card_device.SetOcrAndHighCapacity(ocr);

                /* Handle uhs i mode. */
                this->sd_card_device.SetUhsIMode(false);
                if (uhs_i_supported && ((ocr & OcrSwitchingTo1_8VAccepted) != 0)) {
                    R_TRY(this->EnterUhsIMode());
                    this->sd_card_device.SetUhsIMode(true);
                }

                return ResultSuccess();
            }

            /* Check if we've timed out. */
            R_UNLESS(timer.Update(), sdmmc::ResultSdCardInitializationSoftwareTimeout());

            /* Try again in 1ms. */
            WaitMicroSeconds(1000);
        }
    }

    Result SdCardDeviceAccessor::ChangeToStbyStateAndGetRca() {
        /* Be prepared to wait up to 1.0 seconds to change state. */
        ManualTimer timer(1000);
        while (true) {
            /* Get rca. */
            u16 rca;
            R_TRY(this->IssueCommandSendRelativeAddr(std::addressof(rca)));
            if (rca != 0) {
                this->sd_card_device.SetRca(rca);
                return ResultSuccess();
            }

            /* Check if we've timed out. */
            R_UNLESS(timer.Update(), sdmmc::ResultSdCardGetValidRcaSoftwareTimeout());
        }
    }

    Result SdCardDeviceAccessor::SetMemoryCapacity(const void *csd) {
        if (IsLessThanCsdVersion2(static_cast<const u8 *>(csd))) {
            R_TRY(this->sd_card_device.SetLegacyMemoryCapacity());
        } else {
            AMS_ABORT_UNLESS(util::IsAligned(reinterpret_cast<uintptr_t>(csd), alignof(u16)));
            this->sd_card_device.SetMemoryCapacity(GetMemoryCapacityFromCsd(static_cast<const u16 *>(csd)));
        }

        return ResultSuccess();
    }

    Result SdCardDeviceAccessor::GetScr(void *dst, size_t dst_size) const {
        /* Issue the application command. */
        R_TRY(this->IssueCommandAppCmd(DeviceState_Tran));
        R_TRY(this->IssueCommandSendScr(dst, dst_size));

        return ResultSuccess();
    }

    Result SdCardDeviceAccessor::ExtendBusWidth(BusWidth max_bw, u8 sd_bw) {
        /* If the maximum bus width is 1bit, we can't extend. */
        R_SUCCEED_IF(max_bw == BusWidth_1Bit);

        /* If 4bit mode isn't supported, we can't extend. */
        R_SUCCEED_IF(!IsSupportedBusWidth4Bit(sd_bw));

        /* If the host controller doesn't support 4bit mode, we can't extend. */
        IHostController *hc = BaseDeviceAccessor::GetHostController();
        R_SUCCEED_IF(!hc->IsSupportedBusWidth(BusWidth_4Bit));

        /* Issue the application command to change to 4bit mode. */
        R_TRY(this->IssueCommandAppCmd(DeviceState_Tran));
        R_TRY(this->IssueCommandSetBusWidth4Bit());

        /* Set the host controller's bus width. */
        hc->SetBusWidth(BusWidth_4Bit);

        return ResultSuccess();
    }

    Result SdCardDeviceAccessor::SwitchAccessMode(SwitchFunctionAccessMode access_mode, void *wb, size_t wb_size) {
        /* Issue command to check if we can switch access mode. */
        R_TRY(this->IssueCommandSwitchAccessMode(wb, wb_size, false, access_mode));
        R_UNLESS(IsAccessModeInFunctionSelection(static_cast<const u8 *>(wb), access_mode), sdmmc::ResultSdCardCannotSwitchAccessMode());

        /* Check if we can accept the resulting current consumption. */
        constexpr u16 AcceptableCurrentLimit = 800; /* mA */
        R_UNLESS(GetMaximumCurrentConsumption(static_cast<const u8 *>(wb)) < AcceptableCurrentLimit, sdmmc::ResultSdCardUnacceptableCurrentConsumption());

        /* Switch the access mode. */
        R_TRY(this->IssueCommandSwitchAccessMode(wb, wb_size, true, access_mode));
        R_UNLESS(IsAccessModeInFunctionSelection(static_cast<const u8 *>(wb), access_mode), sdmmc::ResultSdCardFailedSwitchAccessMode());

        return ResultSuccess();
    }

    Result SdCardDeviceAccessor::ExtendBusSpeedAtUhsIMode(SpeedMode max_sm, void *wb, size_t wb_size) {
        /* Check that we're in 4bit bus mode. */
        IHostController *hc = BaseDeviceAccessor::GetHostController();
        R_UNLESS(hc->GetBusWidth() == BusWidth_4Bit, sdmmc::ResultSdCardNot4BitBusWidthAtUhsIMode());

        /* Determine what speed mode/access mode we should switch to. */
        R_TRY(this->IssueCommandCheckSupportedFunction(wb, wb_size));
        SwitchFunctionAccessMode target_am;
        SpeedMode target_sm;
        if (max_sm == SpeedMode_SdCardSdr104 && IsSupportedAccessMode(static_cast<const u8 *>(wb), SwitchFunctionAccessMode_Sdr104)) {
            target_am = SwitchFunctionAccessMode_Sdr104;
            target_sm = SpeedMode_SdCardSdr104;
        } else if ((max_sm == SpeedMode_SdCardSdr104 || max_sm == SpeedMode_SdCardSdr50) && IsSupportedAccessMode(static_cast<const u8 *>(wb), SwitchFunctionAccessMode_Sdr50)) {
            target_am = SwitchFunctionAccessMode_Sdr50;
            target_sm = SpeedMode_SdCardSdr50;
        } else {
            return sdmmc::ResultSdCardNotSupportSdr104AndSdr50();
        }

        /* Switch the access mode. */
        R_TRY(this->SwitchAccessMode(target_am, wb, wb_size));

        /* Set the host controller speed mode and perform tuning using command index 19. */
        R_TRY(hc->SetSpeedMode(target_sm));
        R_TRY(hc->Tuning(target_sm, 19));

        /* Check status. */
        R_TRY(BaseDeviceAccessor::IssueCommandSendStatus());

        return ResultSuccess();
    }

    Result SdCardDeviceAccessor::ExtendBusSpeedAtNonUhsIMode(SpeedMode max_sm, bool spec_under_1_1, void *wb, size_t wb_size) {
        /* If the maximum speed is default speed, we have nothing to do. */
        R_SUCCEED_IF(max_sm == SpeedMode_SdCardDefaultSpeed);

        /* Otherwise, if the spec is under 1.1, we have nothing to do. */
        R_SUCCEED_IF(spec_under_1_1);

        /* Otherwise, Check if high speed is supported. */
        R_TRY(this->IssueCommandCheckSupportedFunction(wb, wb_size));
        R_SUCCEED_IF(!IsSupportedAccessMode(static_cast<const u8 *>(wb), SwitchFunctionAccessMode_HighSpeed));

        /* Switch the access mode. */
        R_TRY(this->SwitchAccessMode(SwitchFunctionAccessMode_HighSpeed, wb, wb_size));

        /* Check status. */
        R_TRY(BaseDeviceAccessor::IssueCommandSendStatus());

        /* Set the host controller speed mode. */
        R_TRY(BaseDeviceAccessor::GetHostController()->SetSpeedMode(SpeedMode_SdCardHighSpeed));

        return ResultSuccess();
    }

    Result SdCardDeviceAccessor::GetSdStatus(void *dst, size_t dst_size) const {
        /* Issue the application command. */
        R_TRY(this->IssueCommandAppCmd(DeviceState_Tran));
        R_TRY(this->IssueCommandSdStatus(dst, dst_size));

        return ResultSuccess();
    }

    void SdCardDeviceAccessor::TryDisconnectDat3PullUpResistor() const {
        /* Issue the application command to clear card detect. */
        /* NOTE: Nintendo accepts a failure. */
        if (R_SUCCEEDED(this->IssueCommandAppCmd(DeviceState_Tran))) {
            /* NOTE: Nintendo does not check the result of this. */
            this->IssueCommandClearCardDetect();
        }

        /* NOTE: Nintendo does not check the result of this. */
        BaseDeviceAccessor::IssueCommandSendStatus();
    }

    Result SdCardDeviceAccessor::StartupSdCardDevice(BusWidth max_bw, SpeedMode max_sm, void *wb, size_t wb_size) {
        /* Start up the host controller. */
        IHostController *hc = BaseDeviceAccessor::GetHostController();
        R_TRY(hc->Startup(BusPower_3_3V, BusWidth_1Bit, SpeedMode_SdCardIdentification, false));

        /* Wait 1ms for configuration to take. */
        WaitMicroSeconds(1000);

        /* Wait an additional 74 clocks for configuration to take. */
        WaitClocks(74, hc->GetDeviceClockFrequencyKHz());

        /* Go to idle state. */
        R_TRY(BaseDeviceAccessor::IssueCommandGoIdleState());

        /* Check whether the spec is under 2.0. */
        bool spec_under_2 = false;
        R_TRY_CATCH(this->IssueCommandSendIfCond()) {
            R_CATCH(sdmmc::ResultResponseTimeoutError) { spec_under_2 = true; }
        } R_END_TRY_CATCH;

        /* Set the rca to 0. */
        this->sd_card_device.SetRca(0);

        /* Go to ready state. */
        const bool can_use_uhs_i_mode = (max_bw != BusWidth_1Bit) && (max_sm == SpeedMode_SdCardSdr104 || max_sm == SpeedMode_SdCardSdr50);
        const bool uhs_i_supported    = hc->IsSupportedTuning() && hc->IsSupportedBusPower(BusPower_1_8V);
        R_TRY(this->ChangeToReadyState(spec_under_2, can_use_uhs_i_mode && uhs_i_supported));

        /* Get the CID. */
        R_TRY(BaseDeviceAccessor::IssueCommandAllSendCid(wb, wb_size));
        this->sd_card_device.SetCid(wb, wb_size);

        /* Go to stby state and get the RCA. */
        R_TRY(this->ChangeToStbyStateAndGetRca());

        /* Get the CSD. */
        R_TRY(BaseDeviceAccessor::IssueCommandSendCsd(wb, wb_size));
        this->sd_card_device.SetCsd(wb, wb_size);
        R_TRY(this->SetMemoryCapacity(wb));

        /* Set the host controller speed mode to default if we're not in uhs i mode. */
        if (!this->sd_card_device.IsUhsIMode()) {
            R_TRY(hc->SetSpeedMode(SpeedMode_SdCardDefaultSpeed));
        }

        /* Issue select card command. */
        R_TRY(BaseDeviceAccessor::IssueCommandSelectCard());

        /* Set block length to sector size. */
        R_TRY(BaseDeviceAccessor::IssueCommandSetBlockLenToSectorSize());

        /* Try to disconnect dat3 pullup resistor. */
        TryDisconnectDat3PullUpResistor();

        /* Get the SCR. */
        R_TRY(this->GetScr(wb, wb_size));
        const u8 sd_bw = GetSdBusWidths(static_cast<const u8 *>(wb));
        const bool spec_under_1_1 = IsLessThanSpecification1_1(static_cast<const u8 *>(wb));

        /* Extend the bus width to the largest that we can. */
        R_TRY(this->ExtendBusWidth(max_bw, sd_bw));

        /* Extend the bus speed to as fast as we can. */
        if (this->sd_card_device.IsUhsIMode()) {
            R_TRY(this->ExtendBusSpeedAtUhsIMode(max_sm, wb, wb_size));
        } else {
            R_TRY(this->ExtendBusSpeedAtNonUhsIMode(max_sm, spec_under_1_1, wb, wb_size));
        }

        /* Enable power saving. */
        hc->SetPowerSaving(true);

        return ResultSuccess();
    }

    Result SdCardDeviceAccessor::OnActivate() {
        /* Define the possible startup parameters. */
        constexpr const struct {
            BusWidth bus_width;
            SpeedMode speed_mode;
        } StartupParameters[] = {
            #if defined(AMS_SDMMC_ENABLE_SD_UHS_I)
                { BusWidth_4Bit, SpeedMode_SdCardSdr104       },
                { BusWidth_4Bit, SpeedMode_SdCardSdr104       },
                { BusWidth_4Bit, SpeedMode_SdCardHighSpeed    },
                { BusWidth_4Bit, SpeedMode_SdCardDefaultSpeed },
                { BusWidth_1Bit, SpeedMode_SdCardHighSpeed    },
            #else
                { BusWidth_4Bit, SpeedMode_SdCardHighSpeed    },
                { BusWidth_4Bit, SpeedMode_SdCardHighSpeed    },
                { BusWidth_4Bit, SpeedMode_SdCardDefaultSpeed },
                { BusWidth_1Bit, SpeedMode_SdCardHighSpeed    },
            #endif
        };

        /* Try to start up with each set of parameters. */
        Result result;
        for (int i = 0; i < static_cast<int>(util::size(StartupParameters)); ++i) {
            /* Alias the parameters. */
            const auto &params = StartupParameters[i];

            /* Set our max bus width/speed mode. */
            this->max_bus_width  = params.bus_width;
            this->max_speed_mode = params.speed_mode;

            /* Try to start up the device. */
            result = this->StartupSdCardDevice(this->max_bus_width, this->max_speed_mode, this->work_buffer, this->work_buffer_size);
            if (R_SUCCEEDED(result)) {
                /* If we previously failed to start up the device, log the error correction. */
                if (i != 0) {
                    BaseDeviceAccessor::PushErrorLog(true, "S %d %d:0", this->max_bus_width, this->max_speed_mode);
                    BaseDeviceAccessor::IncrementNumActivationErrorCorrections();
                }

                return ResultSuccess();
            }

            /* Check if we were removed. */
            AMS_SDMMC_CHECK_SD_CARD_REMOVED();

            /* Log that our startup failed. */
            BaseDeviceAccessor::PushErrorLog(false, "S %d %d:%X", this->max_bus_width, this->max_speed_mode, result.GetValue());

            /* Shut down the host controller before we try to start up again. */
            BaseDeviceAccessor::GetHostController()->Shutdown();
        }

        /* We failed to start up with all sets of parameters. */
        /* Check the csd for errors. */
        if (sdmmc::ResultUnexpectedDeviceCsdValue::Includes(result)) {
            u32 csd[DeviceCsdSize / sizeof(u32)];
            this->sd_card_device.GetCsd(csd, sizeof(csd));
            BaseDeviceAccessor::PushErrorLog(false, "%06X%08X%08X%08X", csd[3] & 0x00FFFFFF, csd[2], csd[1], csd[0]);
        }
        BaseDeviceAccessor::PushErrorTimeStamp();

        /* Check if we failed because the sd card is removed. */
        #if defined(AMS_SDMMC_USE_SD_CARD_DETECTOR)
            if (sdmmc::ResultCommunicationNotAttained::Includes(result)) {
                WaitMicroSeconds(this->sd_card_detector->GetDebounceMilliSeconds() * 1000);
                AMS_SDMMC_CHECK_SD_CARD_REMOVED();
            }
        #endif

        return result;
    }

    Result SdCardDeviceAccessor::OnReadWrite(u32 sector_index, u32 num_sectors, void *buf, size_t buf_size, bool is_read) {
        /* Do the read/write. */
        const Result result = BaseDeviceAccessor::ReadWriteMultiple(sector_index, num_sectors, 0, buf, buf_size, is_read);

        /* Check if we failed because the sd card is removed. */
        #if defined(AMS_SDMMC_USE_SD_CARD_DETECTOR)
            if (sdmmc::ResultCommunicationNotAttained::Includes(result)) {
                WaitMicroSeconds(this->sd_card_detector->GetDebounceMilliSeconds() * 1000);
                AMS_SDMMC_CHECK_SD_CARD_REMOVED();
            }
        #endif

        return result;
    }

    Result SdCardDeviceAccessor::ReStartup() {
        /* Shut down the host controller. */
        BaseDeviceAccessor::GetHostController()->Shutdown();

        /* Perform start up. */
        Result result = this->StartupSdCardDevice(this->max_bus_width, this->max_speed_mode, this->work_buffer, this->work_buffer_size);
        if (R_FAILED(result)) {
            AMS_SDMMC_CHECK_SD_CARD_REMOVED();

            BaseDeviceAccessor::PushErrorLog(false, "S %d %d:%X", this->max_bus_width, this->max_speed_mode, result.GetValue());
            return result;
        }

        return ResultSuccess();
    }

    void SdCardDeviceAccessor::Initialize() {
        /* Acquire exclusive access to the device. */
        AMS_SDMMC_LOCK_SD_CARD_DEVICE_MUTEX();

        /* If we've already initialized, we don't need to do anything. */
        if (this->is_initialized) {
            return;
        }

        /* Set the base device to our sd card device. */
        BaseDeviceAccessor::SetDevice(std::addressof(this->sd_card_device));

        /* Initialize. */
        IHostController *hc = BaseDeviceAccessor::GetHostController();
        #if defined(AMS_SDMMC_USE_SD_CARD_DETECTOR)
        {
            /* TODO: We probably want this (and other sd card detection stuff) to be conditional pcv control active. */
            /*       This will be a requirement to support sd card access with detector in stratosphere before PCV is alive. */
            this->sd_card_device.InitializeRemovedEvent();
            hc->PreSetRemovedEvent(this->sd_card_device.GetRemovedEvent());
            CallbackInfo ci = {
                .inserted_callback     = nullptr,
                .inserted_callback_arg = this,
                .removed_callback      = RemovedCallbackEntry,
                .removed_callback_arg  = this,
            };
            this->sd_card_detector->Initialize(std::addressof(ci));
        }
        #endif
        hc->Initialize();

        /* Mark ourselves as initialized. */
        this->is_initialized = true;
    }

    void SdCardDeviceAccessor::Finalize() {
        /* Acquire exclusive access to the device. */
        AMS_SDMMC_LOCK_SD_CARD_DEVICE_MUTEX();

        /* If we've already finalized, we don't need to do anything. */
        if (!this->is_initialized) {
            return;
        }
        this->is_initialized = false;

        /* Deactivate the device. */
        BaseDeviceAccessor::Deactivate();

        /* Finalize the host controller. */
        BaseDeviceAccessor::GetHostController()->Finalize();

        /* Finalize the detector. */
        #if defined(AMS_SDMMC_USE_SD_CARD_DETECTOR)
        {
            this->sd_card_detector->Finalize();
            this->sd_card_device.FinalizeRemovedEvent();
        }
        #endif
    }

    Result SdCardDeviceAccessor::Activate() {
        /* Activate the detector. */
        #if defined(AMS_SDMMC_USE_SD_CARD_DETECTOR)
        {
            /* Acquire exclusive access to the device. */
            AMS_SDMMC_LOCK_SD_CARD_DEVICE_MUTEX();

            /* Check that we're awake. */
            R_UNLESS(this->sd_card_device.IsAwake(), sdmmc::ResultNotAwakened());

            /* Check that we're not already active. */
            R_SUCCEED_IF(this->sd_card_device.IsActive());

            /* Clear the removed event. */
            this->sd_card_device.ClearRemovedEvent();

            /* Check that the SD card is inserted. */
            R_UNLESS(this->sd_card_detector->IsInserted(), sdmmc::ResultNoDevice());
        }
        #endif

        /* Activate the base device. */
        return BaseDeviceAccessor::Activate();
    }

    Result SdCardDeviceAccessor::GetSpeedMode(SpeedMode *out_speed_mode) const {
        /* Check that we can write to output. */
        AMS_ABORT_UNLESS(out_speed_mode != nullptr);

        /* Acquire exclusive access to the device. */
        AMS_SDMMC_LOCK_SD_CARD_DEVICE_MUTEX();

        /* Check that we're accessible. */
        R_TRY(this->sd_card_device.CheckAccessible());

        /* Check whether we're specification 1 (and thus default speed). */
        R_TRY(this->GetScr(this->work_buffer, this->work_buffer_size));
        if (IsLessThanSpecification1_1(static_cast<const u8 *>(this->work_buffer))) {
            *out_speed_mode = SpeedMode_SdCardDefaultSpeed;
            return ResultSuccess();
        }

        /* Get the current speed mode. */
        R_TRY(this->IssueCommandCheckSupportedFunction(this->work_buffer, this->work_buffer_size));
        R_TRY(GetCurrentSpeedMode(out_speed_mode, static_cast<const u8 *>(this->work_buffer), this->sd_card_device.IsUhsIMode()));

        return ResultSuccess();
    }

    void SdCardDeviceAccessor::PutSdCardToSleep() {
        /* Acquire exclusive access to the device. */
        AMS_SDMMC_LOCK_SD_CARD_DEVICE_MUTEX();

        /* If the device isn't awake, we don't need to do anything. */
        if (!this->sd_card_device.IsAwake()) {
            return;
        }

        /* Put the device to sleep. */
        this->sd_card_device.PutToSleep();

        /* Put the detector to sleep. */
        #if defined(AMS_SDMMC_USE_SD_CARD_DETECTOR)
        this->sd_card_detector->PutToSleep();
        #endif

        /* If necessary, put the host controller to sleep. */
        #if defined(AMS_SDMMC_USE_SD_CARD_DETECTOR)
        if (this->sd_card_device.IsActive() && !this->sd_card_device.IsRemoved())
        #else
        if (this->sd_card_device.IsActive())
        #endif
        {
            BaseDeviceAccessor::GetHostController()->PutToSleep();
        }
    }

    void SdCardDeviceAccessor::AwakenSdCard() {
        /* Acquire exclusive access to the device. */
        AMS_SDMMC_LOCK_SD_CARD_DEVICE_MUTEX();

        /* If the device is awake, we don't need to do anything. */
        if (this->sd_card_device.IsAwake()) {
            return;
        }

        /* Wake the host controller, if we need to.*/
        bool force_det = false;

        #if defined(AMS_SDMMC_USE_SD_CARD_DETECTOR)
        if (this->sd_card_device.IsActive() && !this->sd_card_device.IsRemoved())
        #else
        if (this->sd_card_device.IsActive())
        #endif
        {
            const Result result = BaseDeviceAccessor::GetHostController()->Awaken();
            if (R_SUCCEEDED(result)) {
                force_det = R_FAILED(BaseDeviceAccessor::IssueCommandSendStatus());
            } else {
                BaseDeviceAccessor::PushErrorLog(true, "A:%X", result.GetValue());
                force_det = true;
            }
        }

        /* Wake the detector. */
        #if defined(AMS_SDMMC_USE_SD_CARD_DETECTOR)
        this->sd_card_detector->Awaken(force_det);
        #else
        AMS_UNUSED(force_det);
        #endif

        /* Wake the device. */
        this->sd_card_device.Awaken();
    }

    Result SdCardDeviceAccessor::GetSdCardScr(void *dst, size_t dst_size) const {
        /* Acquire exclusive access to the device. */
        AMS_SDMMC_LOCK_SD_CARD_DEVICE_MUTEX();

        /* Check that we're accessible. */
        R_TRY(this->sd_card_device.CheckAccessible());

        /* Get the SCR. */
        R_TRY(this->GetScr(dst, dst_size));

        return ResultSuccess();
    }

    Result SdCardDeviceAccessor::GetSdCardSwitchFunctionStatus(void *dst, size_t dst_size, SdCardSwitchFunction switch_function) const {
        /* Acquire exclusive access to the device. */
        AMS_SDMMC_LOCK_SD_CARD_DEVICE_MUTEX();

        /* Check that we're accessible. */
        R_TRY(this->sd_card_device.CheckAccessible());

        /* Check whether we're specification 1 (and thus can't switch). */
        R_TRY(this->GetScr(dst, dst_size));
        R_UNLESS(!IsLessThanSpecification1_1(static_cast<const u8 *>(dst)), sdmmc::ResultSdCardNotSupportSwitchFunctionStatus());

        /* Get the status. */
        if (switch_function == SdCardSwitchFunction_CheckSupportedFunction) {
            R_TRY(this->IssueCommandCheckSupportedFunction(dst, dst_size));
        } else {
            SwitchFunctionAccessMode am;
            switch (switch_function) {
                case SdCardSwitchFunction_CheckDefault:   am = SwitchFunctionAccessMode_Default;   break;
                case SdCardSwitchFunction_CheckHighSpeed: am = SwitchFunctionAccessMode_HighSpeed; break;
                case SdCardSwitchFunction_CheckSdr50:     am = SwitchFunctionAccessMode_Sdr50;     break;
                case SdCardSwitchFunction_CheckSdr104:    am = SwitchFunctionAccessMode_Sdr104;    break;
                case SdCardSwitchFunction_CheckDdr50:     am = SwitchFunctionAccessMode_Ddr50;     break;
                AMS_UNREACHABLE_DEFAULT_CASE();
            }

            R_TRY(this->IssueCommandSwitchAccessMode(dst, dst_size, false, am));
        }

        return ResultSuccess();
    }

    Result SdCardDeviceAccessor::GetSdCardCurrentConsumption(u16 *out_current_consumption, SpeedMode speed_mode) const {
        /* Acquire exclusive access to the device. */
        AMS_SDMMC_LOCK_SD_CARD_DEVICE_MUTEX();

        /* Check that we're accessible. */
        R_TRY(this->sd_card_device.CheckAccessible());

        /* Check whether we're specification 1 (and thus can't switch). */
        R_TRY(this->GetScr(this->work_buffer, this->work_buffer_size));
        R_UNLESS(!IsLessThanSpecification1_1(static_cast<const u8 *>(this->work_buffer)), sdmmc::ResultSdCardNotSupportSwitchFunctionStatus());

        /* Determine the access mode. */
        SwitchFunctionAccessMode am;
        switch (speed_mode) {
            case SpeedMode_SdCardSdr12:
            case SpeedMode_SdCardDefaultSpeed:
                am = SwitchFunctionAccessMode_Default;
                break;
            case SpeedMode_SdCardSdr25:
            case SpeedMode_SdCardHighSpeed:
                am = SwitchFunctionAccessMode_HighSpeed;
                break;
            case SpeedMode_SdCardSdr50:
                am = SwitchFunctionAccessMode_Sdr50;
                break;
            case SpeedMode_SdCardSdr104:
                am = SwitchFunctionAccessMode_Sdr104;
                break;
            case SpeedMode_SdCardDdr50:
                am = SwitchFunctionAccessMode_Ddr50;
                break;
            AMS_UNREACHABLE_DEFAULT_CASE();
        }

        /* Check that the mode is supported. */
        R_TRY(this->IssueCommandSwitchAccessMode(this->work_buffer, this->work_buffer_size, false, am));
        R_UNLESS(IsSupportedAccessMode(static_cast<const u8 *>(this->work_buffer), am), sdmmc::ResultSdCardNotSupportAccessMode());

        /* Get the current consumption. */
        AMS_ABORT_UNLESS(out_current_consumption != nullptr);
        *out_current_consumption = GetMaximumCurrentConsumption(static_cast<const u8 *>(this->work_buffer));

        return ResultSuccess();
    }

    Result SdCardDeviceAccessor::GetSdCardSdStatus(void *dst, size_t dst_size) const {
        /* Acquire exclusive access to the device. */
        AMS_SDMMC_LOCK_SD_CARD_DEVICE_MUTEX();

        /* Check that we're accessible. */
        R_TRY(this->sd_card_device.CheckAccessible());

        /* Get the status. */
        R_TRY(this->GetSdStatus(dst, dst_size));

        return ResultSuccess();
    }

    Result SdCardDeviceAccessor::GetSdCardProtectedAreaCapacity(u32 *out_num_sectors) const {
        AMS_ABORT_UNLESS(out_num_sectors != nullptr);

        /* Get the sd status. */
        R_TRY(this->GetSdCardSdStatus(this->work_buffer, this->work_buffer_size));
        const u32 size_of_protected_area = GetSizeOfProtectedArea(static_cast<const u8 *>(this->work_buffer));

        /* Get the csd. */
        u8 csd[DeviceCsdSize];
        this->sd_card_device.GetCsd(csd, sizeof(csd));

        /* Handle based on csd version. */
        if (IsLessThanCsdVersion2(csd)) {
            /* Get c_size_mult and read_bl_len. */
            u8 c_size_mult, read_bl_len;
            this->sd_card_device.GetLegacyCapacityParameters(std::addressof(c_size_mult), std::addressof(read_bl_len));

            /* Validate the parameters. */
            R_UNLESS((read_bl_len + c_size_mult + 2) >= 9, sdmmc::ResultUnexpectedDeviceCsdValue());

            /* Calculate capacity. */
            *out_num_sectors = size_of_protected_area << ((read_bl_len + c_size_mult + 2) - 9);
        } else {
            /* SIZE_OF_PROTECTED_AREA is in bytes. */
            *out_num_sectors = size_of_protected_area / SectorSize;
        }

        return ResultSuccess();
    }


}
