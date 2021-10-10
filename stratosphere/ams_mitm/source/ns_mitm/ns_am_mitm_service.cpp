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
#include <stratosphere.hpp>
#include "ns_am_mitm_service.hpp"
#include "ns_shim.h"

namespace ams::mitm::ns {

    Result NsAmMitmService::GetApplicationContentPath(const sf::OutBuffer &out_path, ncm::ProgramId application_id, u8 content_type) {
        return nsamGetApplicationContentPathFwd(m_forward_service.get(), out_path.GetPointer(), out_path.GetSize(), static_cast<u64>(application_id), static_cast<NcmContentType>(content_type));
    }

    Result NsAmMitmService::ResolveApplicationContentPath(ncm::ProgramId application_id, u8 content_type) {
        /* Always succeed for web applets asking about HBL. */
        /* This enables hbl html. */
        bool is_hbl;
        if (R_SUCCEEDED(pm::info::IsHblProgramId(&is_hbl, application_id)) && is_hbl) {
            nsamResolveApplicationContentPathFwd(m_forward_service.get(), static_cast<u64>(application_id), static_cast<NcmContentType>(content_type));
            return ResultSuccess();
        }
        return nsamResolveApplicationContentPathFwd(m_forward_service.get(), static_cast<u64>(application_id), static_cast<NcmContentType>(content_type));
    }

    Result NsAmMitmService::GetRunningApplicationProgramId(sf::Out<ncm::ProgramId> out, ncm::ProgramId application_id) {
        return nsamGetRunningApplicationProgramIdFwd(m_forward_service.get(), reinterpret_cast<u64 *>(out.GetPointer()), static_cast<u64>(application_id));
    }

}
