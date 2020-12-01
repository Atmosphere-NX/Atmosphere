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

    }

    Result KInitialProcessReader::MakeCreateProcessParameter(ams::svc::CreateProcessParameter *out, bool enable_aslr) const {
        /* Get and validate addresses/sizes. */
        const uintptr_t rx_address  = this->kip_header->GetRxAddress();
        const size_t    rx_size     = this->kip_header->GetRxSize();
        const uintptr_t ro_address  = this->kip_header->GetRoAddress();
        const size_t    ro_size     = this->kip_header->GetRoSize();
        const uintptr_t rw_address  = this->kip_header->GetRwAddress();
        const size_t    rw_size     = this->kip_header->GetRwSize();
        const uintptr_t bss_address = this->kip_header->GetBssAddress();
        const size_t    bss_size    = this->kip_header->GetBssSize();
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

        using ASType = KAddressSpaceInfo::Type;

        const uintptr_t start_address = rx_address;
        const uintptr_t end_address   = bss_size > 0 ? bss_address + bss_size : rw_address + rw_size;
        const size_t    as_width      = this->Is64BitAddressSpace() ? ((GetTargetFirmware() >= TargetFirmware_2_0_0) ? 39 : 36) : 32;
        const ASType    as_type       = this->Is64BitAddressSpace() ? ((GetTargetFirmware() >= TargetFirmware_2_0_0) ? KAddressSpaceInfo::Type_Map39Bit : KAddressSpaceInfo::Type_MapSmall) : KAddressSpaceInfo::Type_MapSmall;
        const uintptr_t map_start     = KAddressSpaceInfo::GetAddressSpaceStart(as_width, as_type);
        const size_t    map_size      = KAddressSpaceInfo::GetAddressSpaceSize(as_width, as_type);
        const uintptr_t map_end       = map_start + map_size;
        MESOSPHERE_ABORT_UNLESS(start_address == 0);

        /* Set fields in parameter. */
        out->code_address   = map_start + start_address;
        out->code_num_pages = util::AlignUp(end_address - start_address, PageSize) / PageSize;
        out->program_id     = this->kip_header->GetProgramId();
        out->version        = this->kip_header->GetVersion();
        out->flags          = 0;
        MESOSPHERE_ABORT_UNLESS((out->code_address / PageSize) + out->code_num_pages <= (map_end / PageSize));

        /* Copy name field. */
        this->kip_header->GetName(out->name, sizeof(out->name));

        /* Apply ASLR, if needed. */
        if (enable_aslr) {
            const size_t choices = (map_end / KernelAslrAlignment) - (util::AlignUp(out->code_address + out->code_num_pages * PageSize, KernelAslrAlignment) / KernelAslrAlignment);
            out->code_address += KSystemControl::GenerateRandomRange(0, choices) * KernelAslrAlignment;
            out->flags |= ams::svc::CreateProcessFlag_EnableAslr;
        }

        /* Apply other flags. */
        if (this->Is64Bit()) {
            out->flags |= ams::svc::CreateProcessFlag_Is64Bit;
        }
        if (this->Is64BitAddressSpace()) {
            out->flags |= (GetTargetFirmware() >= TargetFirmware_2_0_0) ? ams::svc::CreateProcessFlag_AddressSpace64Bit : ams::svc::CreateProcessFlag_AddressSpace64BitDeprecated;
        } else {
            out->flags |= ams::svc::CreateProcessFlag_AddressSpace32Bit;
        }

        /* All initial processes should disable device address space merge. */
        out->flags |= ams::svc::CreateProcessFlag_DisableDeviceAddressSpaceMerge;

        return ResultSuccess();
    }

    Result KInitialProcessReader::Load(KProcessAddress address, const ams::svc::CreateProcessParameter &params) const {
        /* Clear memory at the address. */
        std::memset(GetVoidPointer(address), 0, params.code_num_pages * PageSize);

        /* Prepare to layout the data. */
        const KProcessAddress rx_address = address + this->kip_header->GetRxAddress();
        const KProcessAddress ro_address = address + this->kip_header->GetRoAddress();
        const KProcessAddress rw_address = address + this->kip_header->GetRwAddress();
        const u8 *rx_binary = reinterpret_cast<const u8 *>(this->kip_header + 1);
        const u8 *ro_binary = rx_binary + this->kip_header->GetRxCompressedSize();
        const u8 *rw_binary = ro_binary + this->kip_header->GetRoCompressedSize();

        /* Copy text. */
        if (util::AlignUp(this->kip_header->GetRxSize(), PageSize)) {
            std::memcpy(GetVoidPointer(rx_address), rx_binary, this->kip_header->GetRxCompressedSize());
            if (this->kip_header->IsRxCompressed()) {
                BlzUncompress(GetVoidPointer(rx_address + this->kip_header->GetRxCompressedSize()));
            }
        }

        /* Copy rodata. */
        if (util::AlignUp(this->kip_header->GetRoSize(), PageSize)) {
            std::memcpy(GetVoidPointer(ro_address), ro_binary, this->kip_header->GetRoCompressedSize());
            if (this->kip_header->IsRoCompressed()) {
                BlzUncompress(GetVoidPointer(ro_address + this->kip_header->GetRoCompressedSize()));
            }
        }

        /* Copy rwdata. */
        if (util::AlignUp(this->kip_header->GetRwSize(), PageSize)) {
            std::memcpy(GetVoidPointer(rw_address), rw_binary, this->kip_header->GetRwCompressedSize());
            if (this->kip_header->IsRwCompressed()) {
                BlzUncompress(GetVoidPointer(rw_address + this->kip_header->GetRwCompressedSize()));
            }
        }

        /* Flush caches. */
        /* NOTE: official kernel does an entire cache flush by set/way here, which is incorrect as other cores are online. */
        /* We will simply flush by virtual address, since that's what ARM says is correct to do. */
        MESOSPHERE_R_ABORT_UNLESS(cpu::FlushDataCache(GetVoidPointer(address), params.code_num_pages * PageSize));
        cpu::InvalidateEntireInstructionCache();

        return ResultSuccess();
    }

    Result KInitialProcessReader::SetMemoryPermissions(KProcessPageTable &page_table, const ams::svc::CreateProcessParameter &params) const {
        const size_t rx_size  = this->kip_header->GetRxSize();
        const size_t ro_size  = this->kip_header->GetRoSize();
        const size_t rw_size  = this->kip_header->GetRwSize();
        const size_t bss_size = this->kip_header->GetBssSize();

        /* Set R-X pages. */
        if (rx_size) {
            const uintptr_t start = this->kip_header->GetRxAddress() + params.code_address;
            R_TRY(page_table.SetProcessMemoryPermission(start, util::AlignUp(rx_size, PageSize), ams::svc::MemoryPermission_ReadExecute));
        }

        /* Set R-- pages. */
        if (ro_size) {
            const uintptr_t start = this->kip_header->GetRoAddress() + params.code_address;
            R_TRY(page_table.SetProcessMemoryPermission(start, util::AlignUp(ro_size, PageSize), ams::svc::MemoryPermission_Read));
        }

        /* Set RW- pages. */
        if (rw_size || bss_size) {
            const uintptr_t start = (rw_size ? this->kip_header->GetRwAddress() : this->kip_header->GetBssAddress()) + params.code_address;
            const uintptr_t end  = (bss_size ? this->kip_header->GetBssAddress() + bss_size : this->kip_header->GetRwAddress() + rw_size) + params.code_address;
            R_TRY(page_table.SetProcessMemoryPermission(start, util::AlignUp(end - start, PageSize), ams::svc::MemoryPermission_ReadWrite));
        }

        return ResultSuccess();
    }

}
