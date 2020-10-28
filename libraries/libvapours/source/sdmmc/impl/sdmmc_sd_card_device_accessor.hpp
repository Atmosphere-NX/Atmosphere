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
#include "sdmmc_base_device_accessor.hpp"

#if defined(AMS_SDMMC_USE_SD_CARD_DETECTOR)
#include "sdmmc_device_detector.hpp"
#endif

namespace ams::sdmmc::impl {

    class SdCardDevice : public BaseDevice {
        private:
            #if defined(AMS_SDMMC_USE_SD_CARD_DETECTOR)
            mutable os::EventType removed_event;
            #endif
            u16 rca;
            bool is_valid_rca;
            bool is_uhs_i_mode;
        public:
            SdCardDevice() : rca(0) {
                this->OnDeactivate();
            }

            virtual void Deactivate() override {
                this->OnDeactivate();
                BaseDevice::Deactivate();
            }

            #if defined(AMS_SDMMC_USE_SD_CARD_DETECTOR)
                virtual os::EventType *GetRemovedEvent() const override {
                    return std::addressof(this->removed_event);
                }
            #elif defined(AMS_SDMMC_USE_OS_EVENTS)
                virtual os::EventType *GetRemovedEvent() const override {
                    /* Mmc can't be removed. */
                    return nullptr;
                }
            #endif

            virtual DeviceType GetDeviceType() const override {
                return DeviceType_SdCard;
            }

            virtual u16 GetRca() const override {
                AMS_ABORT_UNLESS(this->is_valid_rca);
                return this->rca;
            }

            void OnDeactivate() {
                this->is_valid_rca  = false;
                this->is_uhs_i_mode = false;
            }

            void SetRca(u16 v) {
                this->rca          = v;
                this->is_valid_rca = true;
            }

            void SetOcrAndHighCapacity(u32 ocr);

            void SetUhsIMode(bool en) {
                this->is_uhs_i_mode = en;
            }

            bool IsUhsIMode() const {
                return this->is_uhs_i_mode;
            }
    };

    enum SdCardApplicationCommandIndex : std::underlying_type<CommandIndex>::type {
        SdApplicationCommandIndex_SetBusWidth             =  6,

        SdApplicationCommandIndex_SdStatus                = 13,

        SdApplicationCommandIndex_SendNumWriteBlocks      = 22,
        SdApplicationCommandIndex_SetWriteBlockEraseCount = 23,

        SdApplicationCommandIndex_SdSendOpCond            = 41,
        SdApplicationCommandIndex_SetClearCardDetect      = 42,

        SdApplicationCommandIndex_SendScr                 = 51,
    };

    enum SwitchFunctionAccessMode {
        SwitchFunctionAccessMode_Default    = 0,
        SwitchFunctionAccessMode_HighSpeed  = 1,
        SwitchFunctionAccessMode_Sdr50      = 2,
        SwitchFunctionAccessMode_Sdr104     = 3,
        SwitchFunctionAccessMode_Ddr50      = 4,
    };

    class SdCardDeviceAccessor : public BaseDeviceAccessor {
        private:
            SdCardDevice sd_card_device;
            void *work_buffer;
            size_t work_buffer_size;
            #if defined(AMS_SDMMC_USE_SD_CARD_DETECTOR)
            DeviceDetector *sd_card_detector;
            #endif
            BusWidth max_bus_width;
            SpeedMode max_speed_mode;
            bool is_initialized;
        private:
            #if defined(AMS_SDMMC_USE_SD_CARD_DETECTOR)
                void RemovedCallback();

                static void RemovedCallbackEntry(void *arg) {
                    static_cast<SdCardDeviceAccessor *>(arg)->RemovedCallback();
                }
            #endif

            Result IssueCommandSendRelativeAddr(u16 *out_rca) const;
            Result IssueCommandSendIfCond() const;
            Result IssueCommandCheckSupportedFunction(void *dst, size_t dst_size) const;
            Result IssueCommandSwitchAccessMode(void *dst, size_t dst_size, bool set_function, SwitchFunctionAccessMode access_mode) const;
            Result IssueCommandVoltageSwitch() const;
            Result IssueCommandAppCmd(DeviceState expected_state, u32 ignore_mask = 0) const;
            Result IssueCommandSetBusWidth4Bit() const;
            Result IssueCommandSdStatus(void *dst, size_t dst_size) const;
            Result IssueCommandSendOpCond(u32 *out_ocr, bool spec_under_2, bool uhs_i_supported) const;
            Result IssueCommandClearCardDetect() const;
            Result IssueCommandSendScr(void *dst, size_t dst_size) const;

            Result EnterUhsIMode();
            Result ChangeToReadyState(bool spec_under_2, bool uhs_i_supported);
            Result ChangeToStbyStateAndGetRca();
            Result SetMemoryCapacity(const void *csd);
            Result GetScr(void *dst, size_t dst_size) const;
            Result ExtendBusWidth(BusWidth max_bw, u8 sd_bw);
            Result SwitchAccessMode(SwitchFunctionAccessMode access_mode, void *wb, size_t wb_size);
            Result ExtendBusSpeedAtUhsIMode(SpeedMode max_sm, void *wb, size_t wb_size);
            Result ExtendBusSpeedAtNonUhsIMode(SpeedMode max_sm, bool spec_under_1_1, void *wb, size_t wb_size);
            Result GetSdStatus(void *dst, size_t dst_size) const;
            Result StartupSdCardDevice(BusWidth max_bw, SpeedMode max_sm, void *wb, size_t wb_size);

            void TryDisconnectDat3PullUpResistor() const;
        protected:
            virtual Result OnActivate() override;
            virtual Result OnReadWrite(u32 sector_index, u32 num_sectors, void *buf, size_t buf_size, bool is_read) override;
            virtual Result ReStartup() override;
        public:
            virtual void Initialize() override;
            virtual void Finalize() override;
            virtual Result Activate() override;
            virtual Result GetSpeedMode(SpeedMode *out_speed_mode) const override;
        public:
            #if defined(AMS_SDMMC_USE_SD_CARD_DETECTOR)
                explicit SdCardDeviceAccessor(IHostController *hc, DeviceDetector *dd) : BaseDeviceAccessor(hc), sd_card_detector(dd)
            #else
                explicit SdCardDeviceAccessor(IHostController *hc) : BaseDeviceAccessor(hc)
            #endif
            {
                this->work_buffer      = nullptr;
                this->work_buffer_size = 0;
                this->max_bus_width    = BusWidth_4Bit;
                this->max_speed_mode   = SpeedMode_SdCardSdr104;
                this->is_initialized   = false;
            }

            void SetSdCardWorkBuffer(void *wb, size_t wb_size) {
                this->work_buffer      = wb;
                this->work_buffer_size = wb_size;
            }

            void PutSdCardToSleep();
            void AwakenSdCard();
            Result GetSdCardProtectedAreaCapacity(u32 *out_num_sectors) const;
            Result GetSdCardScr(void *dst, size_t dst_size) const;
            Result GetSdCardSwitchFunctionStatus(void *dst, size_t dst_size, SdCardSwitchFunction switch_function) const;
            Result GetSdCardCurrentConsumption(u16 *out_current_consumption, SpeedMode speed_mode) const;
            Result GetSdCardSdStatus(void *dst, size_t dst_size) const;

            bool IsSdCardInserted() {
                #if defined(AMS_SDMMC_USE_SD_CARD_DETECTOR)
                    return this->sd_card_detector->IsInserted();
                #else
                    AMS_ABORT("IsSdCardInserted without SdCardDetector");
                #endif
            }

            bool IsSdCardRemoved() {
                #if defined(AMS_SDMMC_USE_SD_CARD_DETECTOR)
                    return this->sd_card_device.IsRemoved();
                #else
                    AMS_ABORT("IsSdCardRemoved without SdCardDetector");
                #endif
            }

            void RegisterSdCardDetectionEventCallback(DeviceDetectionEventCallback cb, void *arg) {
                #if defined(AMS_SDMMC_USE_SD_CARD_DETECTOR)
                    return this->sd_card_detector->RegisterDetectionEventCallback(cb, arg);
                #else
                    AMS_UNUSED(cb, arg);
                    AMS_ABORT("RegisterSdCardDetectionEventCallback without SdCardDetector");
                #endif
            }

            void UnregisterSdCardDetectionEventCallback() {
                #if defined(AMS_SDMMC_USE_SD_CARD_DETECTOR)
                    return this->sd_card_detector->UnregisterDetectionEventCallback();
                #else
                    AMS_ABORT("UnregisterSdCardDetectionEventCallback without SdCardDetector");
                #endif
            }
    };

}
