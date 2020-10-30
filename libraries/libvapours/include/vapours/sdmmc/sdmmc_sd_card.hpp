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

    enum SdCardSwitchFunction {
        SdCardSwitchFunction_CheckSupportedFunction = 0,
        SdCardSwitchFunction_CheckDefault           = 1,
        SdCardSwitchFunction_CheckHighSpeed         = 2,
        SdCardSwitchFunction_CheckSdr50             = 3,
        SdCardSwitchFunction_CheckSdr104            = 4,
        SdCardSwitchFunction_CheckDdr50             = 5,
    };

    constexpr inline size_t SdCardScrSize                  = 0x08;
    constexpr inline size_t SdCardSwitchFunctionStatusSize = 0x40;
    constexpr inline size_t SdCardSdStatusSize             = 0x40;

    constexpr inline size_t SdCardWorkBufferSize           = SdCardSdStatusSize;

    void SetSdCardWorkBuffer(Port port, void *buffer, size_t buffer_size);
    void PutSdCardToSleep(Port port);
    void AwakenSdCard(Port port);
    Result GetSdCardProtectedAreaCapacity(u32 *out_num_sectors, Port port);
    Result GetSdCardScr(void *dst, size_t dst_size, Port port);
    Result GetSdCardSwitchFunctionStatus(void *dst, size_t dst_size, Port port, SdCardSwitchFunction switch_function);
    Result GetSdCardCurrentConsumption(u16 *out_current_consumption, Port port, SpeedMode speed_mode);
    Result GetSdCardSdStatus(void *dst, size_t dst_size, Port port);

    Result CheckSdCardConnection(SpeedMode *out_speed_mode, BusWidth *out_bus_width, Port port);

    bool IsSdCardInserted(Port port);
    bool IsSdCardRemoved(Port port);

    void RegisterSdCardDetectionEventCallback(Port port, DeviceDetectionEventCallback callback, void *arg);
    void UnregisterSdCardDetectionEventCallback(Port port);

}
