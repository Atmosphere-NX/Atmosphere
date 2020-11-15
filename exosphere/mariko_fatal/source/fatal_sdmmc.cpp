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
#include "fatal_device_page_table.hpp"

namespace ams::secmon::fatal {

    namespace {

        constexpr inline auto Port = sdmmc::Port_SdCard0;

        ALWAYS_INLINE u8 *GetSdCardWorkBuffer() {
            return MemoryRegionVirtualDramSdmmcMappedData.GetPointer<u8>() + MemoryRegionVirtualDramSdmmcMappedData.GetSize() - mmu::PageSize;
        }

    }

    Result InitializeSdCard() {
        /* Map main memory for the sdmmc device. */
        AMS_SECMON_LOG("%s\n", "Initializing Device Page Table.");
        InitializeDevicePageTableForSdmmc1();
        AMS_SECMON_LOG("%s\n", "Initialized Device Page Table.");

        /* Initialize sdmmc library. */
        sdmmc::Initialize(Port);
        AMS_SECMON_LOG("%s\n", "Initialized Sdmmc Port.");

        sdmmc::SetSdCardWorkBuffer(Port, GetSdCardWorkBuffer(), sdmmc::SdCardWorkBufferSize);
        AMS_SECMON_LOG("%s\n", "Set SD Card Work Buffer.");

        R_TRY(sdmmc::Activate(Port));
        AMS_SECMON_LOG("%s\n", "Activated.");

        return ResultSuccess();
    }

}
