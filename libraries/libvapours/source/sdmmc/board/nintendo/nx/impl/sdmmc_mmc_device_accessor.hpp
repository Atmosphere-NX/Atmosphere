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

namespace ams::sdmmc::impl {

    class MmcDevice : public BaseDevice {
        private:
            static constexpr u16 Rca = 2;
        public:
            #if defined(AMS_SDMMC_USE_OS_EVENTS)
                virtual os::EventType *GetRemovedEvent() const override {
                    /* Mmc can't be removed. */
                    return nullptr;
                }
            #endif

            virtual DeviceType GetDeviceType() const override {
                return DeviceType_Mmc;
            }

            virtual u16 GetRca() const override {
                return Rca;
            }

            void SetOcrAndHighCapacity(u32 ocr);
    };

    class MmcDeviceAccessor : public BaseDeviceAccessor {
        private:
            MmcDevice mmc_device;
            void *work_buffer;
            size_t work_buffer_size;
            BusWidth max_bus_width;
            SpeedMode max_speed_mode;
            MmcPartition current_partition;
            bool is_initialized;
        private:
            enum CommandSwitch {
                CommandSwitch_SetBitsProductionStateAwarenessEnable               =  0,
                CommandSwitch_ClearBitsAutoModeEnable                             =  1,
                CommandSwitch_WriteProductionStateAwarenessNormal                 =  2,
                CommandSwitch_WriteProductionStateAwarenessPreSolderingWrites     =  3,
                CommandSwitch_WriteProductionStateAwarenessPreSolderingPostWrites =  4,
                CommandSwitch_SetBitsBkopsEnAutoEn                                =  5,
                CommandSwitch_WriteBusWidth1Bit                                   =  6,
                CommandSwitch_WriteBusWidth4Bit                                   =  7,
                CommandSwitch_WriteBusWidth8Bit                                   =  8,
                CommandSwitch_WriteBusWidth8BitDdr                                =  9,
                CommandSwitch_WriteHsTimingLegacySpeed                            = 10,
                CommandSwitch_WriteHsTimingHighSpeed                              = 11,
                CommandSwitch_WriteHsTimingHs200                                  = 12,
                CommandSwitch_WriteHsTimingHs400                                  = 13,
                CommandSwitch_WritePartitionAccessDefault                         = 14,
                CommandSwitch_WritePartitionAccessRwBootPartition1                = 15,
                CommandSwitch_WritePartitionAccessRwBootPartition2                = 16,
            };

            static constexpr ALWAYS_INLINE u32 GetCommandSwitchArgument(CommandSwitch cs) {
                switch (cs) {
                    case CommandSwitch_SetBitsProductionStateAwarenessEnable:               return 0x01111000;
                    case CommandSwitch_ClearBitsAutoModeEnable:                             return 0x02112000;
                    case CommandSwitch_WriteProductionStateAwarenessNormal:                 return 0x03850000;
                    case CommandSwitch_WriteProductionStateAwarenessPreSolderingWrites:     return 0x03850100;
                    case CommandSwitch_WriteProductionStateAwarenessPreSolderingPostWrites: return 0x03850200;
                    case CommandSwitch_SetBitsBkopsEnAutoEn:                                return 0x01A30200;
                    case CommandSwitch_WriteBusWidth1Bit:                                   return 0x03B70000;
                    case CommandSwitch_WriteBusWidth4Bit:                                   return 0x03B70100;
                    case CommandSwitch_WriteBusWidth8Bit:                                   return 0x03B70200;
                    case CommandSwitch_WriteBusWidth8BitDdr:                                return 0x03B70600;
                    case CommandSwitch_WriteHsTimingLegacySpeed:                            return 0x03B90000;
                    case CommandSwitch_WriteHsTimingHighSpeed:                              return 0x03B90100;
                    case CommandSwitch_WriteHsTimingHs200:                                  return 0x03B90200;
                    case CommandSwitch_WriteHsTimingHs400:                                  return 0x03B90300;
                    case CommandSwitch_WritePartitionAccessDefault:                         return 0x03B30000;
                    case CommandSwitch_WritePartitionAccessRwBootPartition1:                return 0x03B30100;
                    case CommandSwitch_WritePartitionAccessRwBootPartition2:                return 0x03B30200;
                    AMS_UNREACHABLE_DEFAULT_CASE();
                }
            }
        private:
            Result IssueCommandSendOpCond(u32 *out_ocr, BusPower bus_power) const;
            Result IssueCommandSetRelativeAddr() const;
            Result IssueCommandSwitch(CommandSwitch cs) const;
            Result IssueCommandSendExtCsd(void *dst, size_t dst_size) const;
            Result IssueCommandEraseGroupStart(u32 sector_index) const;
            Result IssueCommandEraseGroupEnd(u32 sector_index) const;
            Result IssueCommandErase() const;
            Result CancelToshibaMmcModel();
            Result ChangeToReadyState(BusPower bus_power);
            Result ExtendBusWidth(BusWidth max_bus_width);
            Result EnableBkopsAuto();
            Result ChangeToHighSpeed(bool check_before);
            Result ChangeToHs200();
            Result ChangeToHs400();
            Result ExtendBusSpeed(u8 device_type, SpeedMode max_sm);
            Result StartupMmcDevice(BusWidth max_bw, SpeedMode max_sm, void *wb, size_t wb_size);
        protected:
            virtual Result OnActivate() override;
            virtual Result OnReadWrite(u32 sector_index, u32 num_sectors, void *buf, size_t buf_size, bool is_read) override;
            virtual Result ReStartup() override;
        public:
            virtual void Initialize() override;
            virtual void Finalize() override;
            virtual Result GetSpeedMode(SpeedMode *out_speed_mode) const override;
        public:
            explicit MmcDeviceAccessor(IHostController *hc)
                : BaseDeviceAccessor(hc), work_buffer(nullptr), work_buffer_size(0),
                  max_bus_width(BusWidth_8Bit), max_speed_mode(SpeedMode_MmcHs400), current_partition(MmcPartition_Unknown),
                  is_initialized(false)
            {
                /* ... */
            }

            void SetMmcWorkBuffer(void *wb, size_t wb_size) {
                this->work_buffer      = wb;
                this->work_buffer_size = wb_size;
            }

            void PutMmcToSleep();
            void AwakenMmc();
            Result SelectMmcPartition(MmcPartition part);
            Result EraseMmc();
            Result GetMmcBootPartitionCapacity(u32 *out_num_sectors) const;
            Result GetMmcExtendedCsd(void *dst, size_t dst_size) const;
    };

}
