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

#if defined(AMS_SDMMC_USE_OS_EVENTS)
#include <stratosphere/os.hpp>
#endif

namespace ams::sdmmc::impl {

    enum ResponseType {
        ResponseType_R0 = 0,
        ResponseType_R1 = 1,
        ResponseType_R2 = 2,
        ResponseType_R3 = 3,
        ResponseType_R6 = 4,
        ResponseType_R7 = 5,
    };

    enum TransferDirection {
        TransferDirection_ReadFromDevice = 0,
        TransferDirection_WriteToDevice  = 1,
    };

    enum CommandIndex {
        /* Generic commands. */
        CommandIndex_GoIdleState            =  0,
        CommandIndex_SendOpCond             =  1,
        CommandIndex_AllSendCid             =  2,
        CommandIndex_SendRelativeAddr       =  3,
        CommandIndex_SetRelativeAddr        =  3,
        CommandIndex_SetDsr                 =  4,

        CommandIndex_Switch                 =  6,
        CommandIndex_SelectCard             =  7,
        CommandIndex_DeselectCard           =  7,
        CommandIndex_SendIfCond             =  8,
        CommandIndex_SendExtCsd             =  8,
        CommandIndex_SendCsd                =  9,
        CommandIndex_SendCid                = 10,
        CommandIndex_VoltageSwitch          = 11,
        CommandIndex_StopTransmission       = 12,
        CommandIndex_SendStatus             = 13,
        CommandIndex_SendTaskStatus         = 13,

        CommandIndex_GoInactiveState        = 15,
        CommandIndex_SetBlockLen            = 16,
        CommandIndex_ReadSingleBlock        = 17,
        CommandIndex_ReadMultipleBlock      = 18,
        CommandIndex_SendTuningBlock        = 19,
        CommandIndex_SpeedClassControl      = 20,

        CommandIndex_AddressExtension       = 22,
        CommandIndex_SetBlockCount          = 23,
        CommandIndex_WriteBlock             = 24,
        CommandIndex_WriteMultipleBlock     = 25,

        CommandIndex_ProgramCsd             = 27,
        CommandIndex_SetWriteProt           = 28,
        CommandIndex_ClearWriteProt         = 29,
        CommandIndex_SendWriteProt          = 30,

        CommandIndex_EraseWriteBlockStart   = 32,
        CommandIndex_EraseWriteBlockEnd     = 33,

        CommandIndex_EraseGroupStart        = 35,
        CommandIndex_EraseGroupEnd          = 36,

        CommandIndex_Erase                  = 38,

        CommandIndex_LockUnlock             = 42,

        CommandIndex_AppCmd                 = 55,
        CommandIndex_GenCmd                 = 56,

        /* Nintendo specific vendor commands for lotus3. */
        CommandIndex_GcAsicWriteOperation   = 60,
        CommandIndex_GcAsicFinishOperation  = 61,
        CommandIndex_GcAsicSleep            = 62,
        CommandIndex_GcAsicUpdateKey        = 63,
    };

    struct Command {
        u32 command_index;
        u32 command_argument;
        ResponseType response_type;
        bool is_busy;

        constexpr Command(u32 ci, u32 ca, ResponseType r, bool b) : command_index(ci), command_argument(ca), response_type(r), is_busy(b) { /* ... */ }
    };

    struct TransferData {
        void *buffer;
        size_t block_size;
        u32 num_blocks;
        TransferDirection transfer_direction;
        bool is_multi_block_transfer;
        bool is_stop_transmission_command_enabled;

        constexpr TransferData(void *b, size_t bs, u32 nb, TransferDirection xd, bool mb, bool st)
            : buffer(b), block_size(bs),  num_blocks(nb), transfer_direction(xd), is_multi_block_transfer(mb), is_stop_transmission_command_enabled(st)
        {
            if (this->num_blocks > 1) {
                AMS_ABORT_UNLESS(this->is_multi_block_transfer);
            }
        }

        constexpr TransferData(void *b, size_t bs, u32 nb, TransferDirection xd)
            : buffer(b), block_size(bs), num_blocks(nb), transfer_direction(xd), is_multi_block_transfer(false), is_stop_transmission_command_enabled(false)
        {
            AMS_ABORT_UNLESS(this->num_blocks == 1);
        }
    };

    class IHostController {
        public:
            #if defined(AMS_SDMMC_USE_OS_EVENTS)
            virtual void PreSetRemovedEvent(ams::os::EventType *event) = 0;
            #endif

            virtual void Initialize() = 0;
            virtual void Finalize() = 0;

            #if defined(AMS_SDMMC_USE_DEVICE_VIRTUAL_ADDRESS)
            virtual void RegisterDeviceVirtualAddress(uintptr_t buffer, size_t buffer_size, ams::dd::DeviceVirtualAddress buffer_device_virtual_address) = 0;
            virtual void UnregisterDeviceVirtualAddress(uintptr_t buffer, size_t buffer_size, ams::dd::DeviceVirtualAddress buffer_device_virtual_address) = 0;
            #endif

            virtual void SetWorkBuffer(void *wb, size_t wb_size) = 0;

            virtual Result Startup(BusPower bus_power, BusWidth bus_width, SpeedMode speed_mode, bool power_saving_enable) = 0;
            virtual void Shutdown()   = 0;
            virtual void PutToSleep() = 0;
            virtual Result Awaken()   = 0;

            virtual Result SwitchToSdr12();

            virtual bool IsSupportedBusPower(BusPower bus_power) const = 0;
            virtual BusPower GetBusPower() const = 0;

            virtual bool IsSupportedBusWidth(BusWidth bus_width) const = 0;
            virtual void SetBusWidth(BusWidth bus_width) = 0;
            virtual BusWidth GetBusWidth() const = 0;

            virtual Result SetSpeedMode(SpeedMode speed_mode) = 0;
            virtual SpeedMode GetSpeedMode() const = 0;

            virtual u32 GetDeviceClockFrequencyKHz() const = 0;

            virtual void SetPowerSaving(bool en) = 0;
            virtual bool IsPowerSavingEnable() const = 0;

            virtual void EnableDeviceClock() = 0;
            virtual void DisableDeviceClock() = 0;
            virtual bool IsDeviceClockEnable() const = 0;

            virtual u32 GetMaxTransferNumBlocks() const = 0;

            virtual void ChangeCheckTransferInterval(u32 ms) = 0;
            virtual void SetDefaultCheckTransferInterval() = 0;

            virtual Result IssueCommand(const Command *command, TransferData *xfer_data, u32 *out_num_transferred_blocks) = 0;
            virtual Result IssueStopTransmissionCommand(u32 *out_response) = 0;

            ALWAYS_INLINE Result IssueCommand(const Command *command, TransferData *xfer_data) {
                return this->IssueCommand(command, xfer_data, nullptr);
            }

            ALWAYS_INLINE Result IssueCommand(const Command *command) {
                return this->IssueCommand(command, nullptr, nullptr);
            }

            virtual void GetLastResponse(u32 *out_response, size_t response_size, ResponseType response_type) const = 0;
            virtual void GetLastStopTransmissionResponse(u32 *out_response, size_t response_size) const = 0;

            virtual bool IsSupportedTuning() const = 0;
            virtual Result Tuning(SpeedMode speed_mode, u32 command_index) = 0;
            virtual void SaveTuningStatusForHs400() = 0;
            virtual Result GetInternalStatus() const = 0;
    };

}
