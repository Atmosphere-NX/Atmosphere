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
#include <stratosphere.hpp>
#include "ns_web_mitm_service.hpp"

namespace ams::mitm::ns {

    Result NsDocumentService::GetApplicationContentPath(const sf::OutBuffer &out_path, ncm::ProgramId application_id, u8 content_type) {
        return nswebGetApplicationContentPath(this->srv.get(), out_path.GetPointer(), out_path.GetSize(), static_cast<u64>(application_id), static_cast<NcmContentType>(content_type));
    }

    Result NsDocumentService::ResolveApplicationContentPath(ncm::ProgramId application_id, u8 content_type) {
        /* Always succeed for web applets asking about HBL. */
        /* This enables hbl html. */
        bool is_hbl;
        if (R_SUCCEEDED(pm::info::IsHblProgramId(&is_hbl, application_id)) && is_hbl) {
            nswebResolveApplicationContentPath(this->srv.get(), static_cast<u64>(application_id), static_cast<NcmContentType>(content_type));
            return ResultSuccess();
        }
        return nswebResolveApplicationContentPath(this->srv.get(), static_cast<u64>(application_id), static_cast<NcmContentType>(content_type));
    }

    Result NsDocumentService::GetRunningApplicationProgramId(sf::Out<ncm::ProgramId> out, ncm::ProgramId application_id) {
        return nswebGetRunningApplicationProgramId(this->srv.get(), reinterpret_cast<u64 *>(out.GetPointer()), static_cast<u64>(application_id));
    }

    Result NsWebMitmService::GetDocumentInterface(sf::Out<std::shared_ptr<impl::IDocumentInterface>> out) {
        /* Open a document interface. */
        NsDocumentInterface doc;
        R_TRY(nsGetDocumentInterfaceFwd(this->forward_service.get(), &doc));
        const sf::cmif::DomainObjectId target_object_id{serviceGetObjectId(&doc.s)};

        out.SetValue(sf::MakeShared<impl::IDocumentInterface, NsDocumentService>(this->client_info, std::make_unique<NsDocumentInterface>(doc)), target_object_id);
        return ResultSuccess();
    }

}
