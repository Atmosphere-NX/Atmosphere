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
#include <vapours/sdmmc/sdmmc_build_config.hpp>

namespace ams::sdmmc {

    enum BusPower {
        BusPower_Off  = 0,
        BusPower_1_8V = 1,
        BusPower_3_3V = 2,
    };

    enum BusWidth {
        BusWidth_1Bit = 0,
        BusWidth_4Bit = 1,
        BusWidth_8Bit = 2,
    };

    enum SpeedMode {
        SpeedMode_MmcIdentification     =  0,
        SpeedMode_MmcLegacySpeed        =  1,
        SpeedMode_MmcHighSpeed          =  2,
        SpeedMode_MmcHs200              =  3,
        SpeedMode_MmcHs400              =  4,
        SpeedMode_SdCardIdentification  =  5,
        SpeedMode_SdCardDefaultSpeed    =  6,
        SpeedMode_SdCardHighSpeed       =  7,
        SpeedMode_SdCardSdr12           =  8,
        SpeedMode_SdCardSdr25           =  9,
        SpeedMode_SdCardSdr50           = 10,
        SpeedMode_SdCardSdr104          = 11,
        SpeedMode_SdCardDdr50           = 12,
        SpeedMode_GcAsicFpgaSpeed       = 13,
        SpeedMode_GcAsicSpeed           = 14,
    };

    enum Port {
        Port_Mmc0 = 0,
        Port_SdCard0 = 1,
        Port_GcAsic0 = 2,
    };

    struct ErrorInfo {
        u32 num_activation_failures;
        u32 num_activation_error_corrections;
        u32 num_read_write_failures;
        u32 num_read_write_error_corrections;
    };

    struct DataTransfer {
        void *buffer;
        size_t buffer_size;
        size_t block_size;
        u32 num_blocks;
        bool is_read;
    };

    using DeviceDetectionEventCallback = void (*)(void *);

    constexpr inline size_t SectorSize    = 0x200;

    constexpr inline size_t DeviceCidSize = 0x10;
    constexpr inline size_t DeviceCsdSize = 0x10;

    constexpr inline size_t BufferDeviceVirtualAddressAlignment = alignof(ams::dd::DeviceVirtualAddress);
    static_assert(BufferDeviceVirtualAddressAlignment >= 8);

    void Initialize(Port port);
    void Finalize(Port port);

#if defined(AMS_SDMMC_USE_PCV_CLOCK_RESET_CONTROL)
    void SwitchToPcvClockResetControl();
#endif

#if defined(AMS_SDMMC_USE_DEVICE_VIRTUAL_ADDRESS)
    void RegisterDeviceVirtualAddress(Port port, uintptr_t buffer, size_t buffer_size, ams::dd::DeviceVirtualAddress buffer_device_virtual_address);
    void UnregisterDeviceVirtualAddress(Port port, uintptr_t buffer, size_t buffer_size, ams::dd::DeviceVirtualAddress buffer_device_virtual_address);
#endif

    void ChangeCheckTransferInterval(Port port, u32 ms);
    void SetDefaultCheckTransferInterval(Port port);

    Result Activate(Port port);
    void Deactivate(Port port);

    Result Read(void *dst, size_t dst_size, Port port, u32 sector_index, u32 num_sectors);
    Result Write(Port port, u32 sector_index, u32 num_sectors, const void *src, size_t src_size);

    Result CheckConnection(SpeedMode *out_speed_mode, BusWidth *out_bus_width, Port port);

    Result GetDeviceSpeedMode(SpeedMode *out, Port port);
    Result GetDeviceMemoryCapacity(u32 *out_num_sectors, Port port);
    Result GetDeviceStatus(u32 *out_device_status, Port port);
    Result GetDeviceCid(void *out, size_t out_size, Port port);
    Result GetDeviceCsd(void *out, size_t out_size, Port port);

    void GetAndClearErrorInfo(ErrorInfo *out_error_info, size_t *out_log_size, char *out_log_buffer, size_t log_buffer_size, Port port);

}