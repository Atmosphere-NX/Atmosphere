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
#include <mesosphere.hpp>

namespace ams::kern {

    namespace {

        struct BlzSegmentFlags {
            using Offset = util::BitPack16::Field<0,            12, u32>;
            using Size   = util::BitPack16::Field<Offset::Next,  4, u32>;
        };

        NOINLINE void BlzUncompress(void *_end) {
            /* Parse the footer, endian agnostic. */
            static_assert(sizeof(u32) == 4);
            static_assert(sizeof(u16) == 2);
            static_assert(sizeof(u8)  == 1);

            u8 *end = static_cast<u8 *>(_end);
            const u32 total_size      = (end[-12] << 0) | (end[-11] << 8) | (end[-10] << 16) | (end[- 9] << 24);
            const u32 footer_size     = (end[- 8] << 0) | (end[- 7] << 8) | (end[- 6] << 16) | (end[- 5] << 24);
            const u32 additional_size = (end[- 4] << 0) | (end[- 3] << 8) | (end[- 2] << 16) | (end[- 1] << 24);

            /* Prepare to decompress. */
            u8 *cmp_start = end - total_size;
            u32 cmp_ofs = total_size - footer_size;
            u32 out_ofs = total_size + additional_size;

            /* Decompress. */
            while (out_ofs) {
                u8 control = cmp_start[--cmp_ofs];

                /* Each bit in the control byte is a flag indicating compressed or not compressed. */
                for (size_t i = 0; i < 8 && out_ofs; ++i, control <<= 1) {
                    if (control & 0x80) {
                        /* NOTE: Nintendo does not check if it's possible to decompress. */
                        /* As such, we will leave the following as a debug assertion, and not a release assertion. */
                        MESOSPHERE_AUDIT(cmp_ofs >= sizeof(u16));
                        cmp_ofs -= sizeof(u16);

                        /* Extract segment bounds. */
                        const util::BitPack16 seg_flags{static_cast<u16>((cmp_start[cmp_ofs] << 0) | (cmp_start[cmp_ofs + 1] << 8))};
                        const u32 seg_ofs  = seg_flags.Get<BlzSegmentFlags::Offset>() + 3;
                        const u32 seg_size = std::min(seg_flags.Get<BlzSegmentFlags::Size>() + 3, out_ofs);
                        MESOSPHERE_AUDIT(out_ofs + seg_ofs <= total_size + additional_size);

                        /* Copy the data. */
                        out_ofs -= seg_size;
                        for (size_t j = 0; j < seg_size; j++) {
                            cmp_start[out_ofs + j] = cmp_start[out_ofs + seg_ofs + j];
                        }
                    } else {
                        /* NOTE: Nintendo does not check if it's possible to copy. */
                        /* As such, we will leave the following as a debug assertion, and not a release assertion. */
                        MESOSPHERE_AUDIT(cmp_ofs >= sizeof(u8));
                        cmp_start[--out_ofs] = cmp_start[--cmp_ofs];
                    }
                }
            }
        }

        NOINLINE void LoadInitialProcessSegment(const KPageGroup &pg, size_t seg_offset, size_t seg_size, size_t binary_size, KVirtualAddress data, bool compressed) {
            /* Save the original binary extents, for later use. */
            const KPhysicalAddress binary_phys = KMemoryLayout::GetLinearPhysicalAddress(data);

            /* Create a page group representing the segment. */
            KPageGroup segment_pg(Kernel::GetSystemSystemResource().GetBlockInfoManagerPointer());
            MESOSPHERE_R_ABORT_UNLESS(pg.CopyRangeTo(segment_pg, seg_offset, util::AlignUp(seg_size, PageSize)));

            /* Setup the new page group's memory so that we can load the segment. */
            {
                KVirtualAddress last_block = Null<KVirtualAddress>;
                KVirtualAddress last_data  = Null<KVirtualAddress>;
                size_t last_copy_size      = 0;
                size_t last_clear_size     = 0;
                size_t remaining_copy_size = binary_size;
                for (const auto &block : segment_pg) {
                    /* Get the current block extents. */
                    const auto block_addr   = block.GetAddress();
                    const size_t block_size = block.GetSize();
                    if (remaining_copy_size > 0) {
                        /* Determine if we need to copy anything. */
                        const size_t cur_size = std::min<size_t>(block_size, remaining_copy_size);

                        /* NOTE: The first block may potentially overlap the binary we want to copy to. */
                        /* Consider e.g. the case where the overall compressed image has size 0x40000, seg_offset is 0x30000, and binary_size is > 0x20000. */
                        /* Suppose too that data points, say, 0x18000 into the compressed image. */
                        /* Suppose finally that we simply naively copy in order. */
                        /* The first iteration of this loop will perform an 0x10000 copy from image+0x18000 to image + 0x30000 (as there is no overlap). */
                        /* The second iteration will perform a copy from image+0x28000 to <allocated pages>. */
                        /* However, the first copy will have trashed the data in the second copy. */
                        /* Thus, we must copy the first block after-the-fact to avoid potentially trashing data in the overlap case. */
                        /* It is guaranteed by pre-condition that only the very first block can overlap with the physical binary, so we can simply memmove it at the end. */
                        if (last_block != Null<KVirtualAddress>) {
                            /* This is guaranteed by pre-condition, but for ease of debugging, check for no overlap. */
                            MESOSPHERE_ASSERT(!util::HasOverlap(GetInteger(binary_phys), binary_size, GetInteger(block_addr), cur_size));
                            MESOSPHERE_UNUSED(binary_phys);

                            /* We need to copy. */
                            std::memcpy(GetVoidPointer(KMemoryLayout::GetLinearVirtualAddress(block_addr)), GetVoidPointer(data), cur_size);

                            /* If we need to, clear past where we're copying. */
                            if (cur_size != block_size) {
                                std::memset(GetVoidPointer(KMemoryLayout::GetLinearVirtualAddress(block_addr + cur_size)), 0, block_size - cur_size);
                            }

                            /* Advance. */
                            remaining_copy_size -= cur_size;
                            data += cur_size;
                        } else {
                            /* Save the first block, which may potentially overlap, so that we can copy it later. */
                            last_block      = KMemoryLayout::GetLinearVirtualAddress(block_addr);
                            last_data       = data;
                            last_copy_size  = cur_size;
                            last_clear_size = block_size - cur_size;

                            /* Advance. */
                            remaining_copy_size -= cur_size;
                            data += cur_size;
                        }
                    } else {
                        /* We don't have data to copy, so we should just clear the pages. */
                        std::memset(GetVoidPointer(KMemoryLayout::GetLinearVirtualAddress(block_addr)), 0, block_size);
                    }
                }

                /* Handle a last block. */
                if (last_copy_size != 0) {
                    if (last_block != last_data) {
                        std::memmove(GetVoidPointer(last_block), GetVoidPointer(last_data), last_copy_size);
                    }
                    if (last_clear_size != 0) {
                        std::memset(GetVoidPointer(last_block + last_copy_size), 0, last_clear_size);
                    }
                }
            }

            /* If compressed, uncompress the data. */
            if (compressed) {
                /* Get the temporary region. */
                const auto &temp_region = KMemoryLayout::GetTempRegion();
                MESOSPHERE_ABORT_UNLESS(temp_region.GetEndAddress() != 0);

                /* Map the process's memory into the temporary region. */
                KProcessAddress temp_address = Null<KProcessAddress>;
                MESOSPHERE_R_ABORT_UNLESS(Kernel::GetKernelPageTable().MapPageGroup(std::addressof(temp_address), segment_pg, temp_region.GetAddress(), temp_region.GetSize() / PageSize, KMemoryState_Kernel, KMemoryPermission_KernelReadWrite));
                ON_SCOPE_EXIT { MESOSPHERE_R_ABORT_UNLESS(Kernel::GetKernelPageTable().UnmapPageGroup(temp_address, segment_pg, KMemoryState_Kernel)); };

                /* Uncompress the data. */
                BlzUncompress(GetVoidPointer(temp_address + binary_size));
            }
        }

    }

    Result KInitialProcessReader::MakeCreateProcessParameter(ams::svc::CreateProcessParameter *out, bool enable_aslr) const {
        /* Get and validate addresses/sizes. */
        const uintptr_t rx_address  = m_kip_header.GetRxAddress();
        const size_t    rx_size     = m_kip_header.GetRxSize();
        const uintptr_t ro_address  = m_kip_header.GetRoAddress();
        const size_t    ro_size     = m_kip_header.GetRoSize();
        const uintptr_t rw_address  = m_kip_header.GetRwAddress();
        const size_t    rw_size     = m_kip_header.GetRwSize();
        const uintptr_t bss_address = m_kip_header.GetBssAddress();
        const size_t    bss_size    = m_kip_header.GetBssSize();
        R_UNLESS(util::IsAligned(rx_address, PageSize),                          svc::ResultInvalidAddress());
        R_UNLESS(util::IsAligned(ro_address, PageSize),                          svc::ResultInvalidAddress());
        R_UNLESS(util::IsAligned(rw_address, PageSize),                          svc::ResultInvalidAddress());
        R_UNLESS(rx_address  <= rx_address  + util::AlignUp(rx_size, PageSize),  svc::ResultInvalidAddress());
        R_UNLESS(ro_address  <= ro_address  + util::AlignUp(ro_size, PageSize),  svc::ResultInvalidAddress());
        R_UNLESS(rw_address  <= rw_address  + util::AlignUp(rw_size, PageSize),  svc::ResultInvalidAddress());
        R_UNLESS(bss_address <= bss_address + util::AlignUp(bss_size, PageSize), svc::ResultInvalidAddress());
        R_UNLESS(rx_address + util::AlignUp(rx_size, PageSize) <= ro_address,    svc::ResultInvalidAddress());
        R_UNLESS(ro_address + util::AlignUp(ro_size, PageSize) <= rw_address,    svc::ResultInvalidAddress());
        R_UNLESS(rw_address + rw_size                          <= bss_address,   svc::ResultInvalidAddress());

        /* Validate the address space. */
        if (this->Is64BitAddressSpace()) {
            R_UNLESS(this->Is64Bit(), svc::ResultInvalidCombination());
        }

        const uintptr_t start_address = rx_address;
        const uintptr_t end_address   = bss_size > 0 ? bss_address + bss_size : rw_address + rw_size;
        MESOSPHERE_ABORT_UNLESS(start_address == 0);

        /* Default fields in parameter to zero. */
        *out = {};

        /* Set fields in parameter. */
        out->code_address              = 0;
        out->code_num_pages            = util::AlignUp(end_address - start_address, PageSize) / PageSize;
        out->program_id                = m_kip_header.GetProgramId();
        out->version                   = m_kip_header.GetVersion();
        out->flags                     = 0;
        out->reslimit                  = ams::svc::InvalidHandle;
        out->system_resource_num_pages = 0;

        /* Copy name field. */
        m_kip_header.GetName(out->name, sizeof(out->name));

        /* Apply other flags. */
        if (this->Is64Bit()) {
            out->flags |= ams::svc::CreateProcessFlag_Is64Bit;
        }
        if (this->Is64BitAddressSpace()) {
            out->flags |= (GetTargetFirmware() >= TargetFirmware_2_0_0) ? ams::svc::CreateProcessFlag_AddressSpace64Bit : ams::svc::CreateProcessFlag_AddressSpace64BitDeprecated;
        } else {
            out->flags |= ams::svc::CreateProcessFlag_AddressSpace32Bit;
        }
        if (enable_aslr) {
            out->flags |= ams::svc::CreateProcessFlag_EnableAslr;
        }

        /* All initial processes should disable device address space merge. */
        out->flags |= ams::svc::CreateProcessFlag_DisableDeviceAddressSpaceMerge;

        /* Set and check code address. */
        /* NOTE: Even though Nintendo passes a size to GetAddressSpaceStart at other call sites, they pass */
        /* a number of pages here. Even though this is presumably only used for debug assertions, this is */
        /* almost certainly a bug. */
        using ASType = KAddressSpaceInfo::Type;
        const ASType    as_type       = this->Is64BitAddressSpace() ? ((GetTargetFirmware() >= TargetFirmware_2_0_0) ? KAddressSpaceInfo::Type_Map39Bit : KAddressSpaceInfo::Type_MapSmall) : KAddressSpaceInfo::Type_MapSmall;
        const uintptr_t map_start     = KAddressSpaceInfo::GetAddressSpaceStart(static_cast<ams::svc::CreateProcessFlag>(out->flags), as_type, out->code_num_pages);
        const size_t    map_size      = KAddressSpaceInfo::GetAddressSpaceSize(static_cast<ams::svc::CreateProcessFlag>(out->flags), as_type);
        const uintptr_t map_end       = map_start + map_size;
        out->code_address             = map_start + start_address;
        MESOSPHERE_ABORT_UNLESS((out->code_address / PageSize) + out->code_num_pages <= (map_end / PageSize));

        /* Apply ASLR, if needed. */
        if (enable_aslr) {
            const size_t choices = (map_end / KernelAslrAlignment) - (util::AlignUp(out->code_address + out->code_num_pages * PageSize, KernelAslrAlignment) / KernelAslrAlignment);
            out->code_address += KSystemControl::GenerateRandomRange(0, choices) * KernelAslrAlignment;
        }

        R_SUCCEED();
    }

    void KInitialProcessReader::Load(const KPageGroup &pg, KVirtualAddress data) const {
        /* Prepare to layout the data. */
        const KVirtualAddress rx_data = data;
        const KVirtualAddress ro_data = rx_data + m_kip_header.GetRxCompressedSize();
        const KVirtualAddress rw_data = ro_data + m_kip_header.GetRoCompressedSize();
        const size_t rx_size  = m_kip_header.GetRxSize();
        const size_t ro_size  = m_kip_header.GetRoSize();
        const size_t rw_size  = m_kip_header.GetRwSize();

        /* If necessary, setup bss. */
        if (const size_t bss_size = m_kip_header.GetBssSize(); bss_size > 0) {
            /* Determine how many additional pages are needed for bss. */
            const u64 rw_end  = util::AlignUp<u64>(m_kip_header.GetRwAddress() + m_kip_header.GetRwSize(), PageSize);
            const u64 bss_end = util::AlignUp<u64>(m_kip_header.GetBssAddress() + m_kip_header.GetBssSize(), PageSize);
            if (rw_end != bss_end) {
                /* Find the pages corresponding to bss. */
                size_t cur_offset = 0;
                size_t remaining_size = bss_end - rw_end;
                size_t bss_offset = rw_end - m_kip_header.GetRxAddress();
                for (auto it = pg.begin(); it != pg.end() && remaining_size > 0; ++it) {
                    /* Get the current size. */
                    const size_t cur_size = it->GetSize();

                    /* Determine if the offset is in range. */
                    const size_t rel_diff = bss_offset - cur_offset;
                    const bool is_before  = cur_offset <= bss_offset;
                    cur_offset += cur_size;
                    if (is_before && bss_offset < cur_offset) {
                        /* It is, so clear the bss range. */
                        const size_t block_size = std::min<size_t>(cur_size - rel_diff, remaining_size);
                        std::memset(GetVoidPointer(KMemoryLayout::GetLinearVirtualAddress(it->GetAddress() + rel_diff)), 0, block_size);

                        /* Advance. */
                        cur_offset      = bss_offset + block_size;
                        remaining_size -= block_size;
                        bss_offset     += block_size;
                    }
                }
            }
        }

        /* Load .rwdata. */
        LoadInitialProcessSegment(pg, m_kip_header.GetRwAddress() - m_kip_header.GetRxAddress(), rw_size, m_kip_header.GetRwCompressedSize(), rw_data, m_kip_header.IsRwCompressed());

        /* Load .rodata. */
        LoadInitialProcessSegment(pg, m_kip_header.GetRoAddress() - m_kip_header.GetRxAddress(), ro_size, m_kip_header.GetRoCompressedSize(), ro_data, m_kip_header.IsRoCompressed());

        /* Load .text. */
        LoadInitialProcessSegment(pg, m_kip_header.GetRxAddress() - m_kip_header.GetRxAddress(), rx_size, m_kip_header.GetRxCompressedSize(), rx_data, m_kip_header.IsRxCompressed());
    }

    Result KInitialProcessReader::SetMemoryPermissions(KProcessPageTable &page_table, const ams::svc::CreateProcessParameter &params) const {
        const size_t rx_size  = m_kip_header.GetRxSize();
        const size_t ro_size  = m_kip_header.GetRoSize();
        const size_t rw_size  = m_kip_header.GetRwSize();
        const size_t bss_size = m_kip_header.GetBssSize();

        /* Set R-X pages. */
        if (rx_size) {
            const uintptr_t start = m_kip_header.GetRxAddress() + params.code_address;
            R_TRY(page_table.SetProcessMemoryPermission(start, util::AlignUp(rx_size, PageSize), ams::svc::MemoryPermission_ReadExecute));
        }

        /* Set R-- pages. */
        if (ro_size) {
            const uintptr_t start = m_kip_header.GetRoAddress() + params.code_address;
            R_TRY(page_table.SetProcessMemoryPermission(start, util::AlignUp(ro_size, PageSize), ams::svc::MemoryPermission_Read));
        }

        /* Set RW- pages. */
        if (rw_size || bss_size) {
            const uintptr_t start = (rw_size ? m_kip_header.GetRwAddress() : m_kip_header.GetBssAddress()) + params.code_address;
            const uintptr_t end  = (bss_size ? m_kip_header.GetBssAddress() + bss_size : m_kip_header.GetRwAddress() + rw_size) + params.code_address;
            R_TRY(page_table.SetProcessMemoryPermission(start, util::AlignUp(end - start, PageSize), ams::svc::MemoryPermission_ReadWrite));
        }

        R_SUCCEED();
    }

}
