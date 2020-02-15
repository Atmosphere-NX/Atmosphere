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
        const size_t    as_width      = this->Is64BitAddressSpace() ? 39 : 32;
        const ASType    as_type       = this->Is64BitAddressSpace() ? KAddressSpaceInfo::Type_Large64Bit : KAddressSpaceInfo::Type_32Bit;
        const uintptr_t map_start     = KAddressSpaceInfo::GetAddressSpaceStart(as_width, as_type);
        const size_t    map_size      = KAddressSpaceInfo::GetAddressSpaceSize(as_width, as_type);
        const uintptr_t map_end       = map_start + map_size;
        MESOSPHERE_ABORT_UNLESS(start_address == 0);

        /* Set fields in parameter. */
        out->code_address   = map_start + start_address;
        out->code_num_pages = util::AlignUp(end_address - start_address, PageSize);
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
            out->flags |= ams::svc::CreateProcessFlag_AddressSpace64Bit;
        } else {
            out->flags |= ams::svc::CreateProcessFlag_AddressSpace32Bit;
        }

        return ResultSuccess();
    }

}
