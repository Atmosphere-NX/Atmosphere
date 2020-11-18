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

        ALWAYS_INLINE u8 *GetSdCardDmaBuffer() {
            return MemoryRegionVirtualDramSdmmcMappedData.GetPointer<u8>();
        }

        constexpr inline size_t SdCardDmaBufferSize = MemoryRegionVirtualDramSdmmcMappedData.GetSize() - mmu::PageSize;
        constexpr inline size_t SdCardDmaBufferSectors = SdCardDmaBufferSize / sdmmc::SectorSize;
        static_assert(util::IsAligned(SdCardDmaBufferSize, sdmmc::SectorSize));

    }

    Result InitializeSdCard() {
        /* Map main memory for the sdmmc device. */
        InitializeDevicePageTableForSdmmc1();

        /* Initialize sdmmc library. */
        sdmmc::Initialize(Port);

        sdmmc::SetSdCardWorkBuffer(Port, GetSdCardWorkBuffer(), sdmmc::SdCardWorkBufferSize);

        //sdmmc::Deactivate(Port);
        R_TRY(sdmmc::Activate(Port));

        return ResultSuccess();
    }

    Result CheckSdCardConnection(sdmmc::SpeedMode *out_sm, sdmmc::BusWidth *out_bw) {
        return sdmmc::CheckSdCardConnection(out_sm, out_bw, Port);
    }

    Result ReadSdCard(void *dst, size_t size, size_t sector_index, size_t sector_count) {
        /* Validate that our buffer is valid. */
        AMS_ASSERT(size >= sector_count * sdmmc::SectorSize);

        /* Repeatedly read sectors. */
        u8 *dst_u8 = static_cast<u8 *>(dst);
        void * const dma_buffer = GetSdCardDmaBuffer();
        while (sector_count > 0) {
            /* Read sectors into the DMA buffer. */
            const size_t cur_sectors = std::min(sector_count, SdCardDmaBufferSectors);
            const size_t cur_size    = cur_sectors * sdmmc::SectorSize;
            R_TRY(sdmmc::Read(dma_buffer, cur_size, Port, sector_index, cur_sectors));

            /* Copy data from the DMA buffer to the output. */
            std::memcpy(dst_u8, dma_buffer, cur_size);

            /* Advance. */
            dst_u8       += cur_size;
            sector_index += cur_sectors;
            sector_count -= cur_sectors;
        }

        return ResultSuccess();
    }

    Result WriteSdCard(size_t sector_index, size_t sector_count, const void *src, size_t size) {
        /* Validate that our buffer is valid. */
        AMS_ASSERT(size >= sector_count * sdmmc::SectorSize);

        /* Repeatedly read sectors. */
        const u8 *src_u8 = static_cast<const u8 *>(src);
        void * const dma_buffer = GetSdCardDmaBuffer();
        while (sector_count > 0) {
            /* Copy sectors into the DMA buffer. */
            const size_t cur_sectors = std::min(sector_count, SdCardDmaBufferSectors);
            const size_t cur_size    = cur_sectors * sdmmc::SectorSize;
            std::memcpy(dma_buffer, src_u8, cur_size);

            /* Write sectors to the sd card. */
            R_TRY(sdmmc::Write(Port, sector_index, cur_sectors, dma_buffer, cur_size));

            /* Advance. */
            src_u8       += cur_size;
            sector_index += cur_sectors;
            sector_count -= cur_sectors;
        }

        return ResultSuccess();
    }

}
