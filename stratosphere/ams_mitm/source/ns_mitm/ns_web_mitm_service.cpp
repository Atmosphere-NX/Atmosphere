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
#include "../amsmitm_fs_utils.hpp"
#include "ns_web_mitm_service.hpp"

namespace ams::mitm::ns {

    Result NsDocumentService::GetApplicationContentPath(const sf::OutBuffer &out_path, ncm::ProgramId application_id, u8 content_type) {
        R_RETURN(nswebGetApplicationContentPath(m_srv.get(), out_path.GetPointer(), out_path.GetSize(), static_cast<u64>(application_id), static_cast<NcmContentType>(content_type)));
    }

    Result NsDocumentService::ResolveApplicationContentPath(ncm::ProgramId application_id, u8 content_type) {
        /* Always succeed if a `contents/<application_id>/manual_html` directory exists and is not empty. */
        /* This allows displaying HTML in applications without a manual NCA. */
        FsDir web_content_dir;
        if (R_SUCCEEDED(mitm::fs::OpenAtmosphereSdDirectory(std::addressof(web_content_dir), application_id, "manual_html", ams::fs::OpenDirectoryMode_All))) {
            ON_SCOPE_EXIT { fsDirClose(std::addressof(web_content_dir)); };
            s64 num_entries = 0;
            if (R_SUCCEEDED(fsDirGetEntryCount(std::addressof(web_content_dir), std::addressof(num_entries))) && num_entries > 0) {
                nswebResolveApplicationContentPath(m_srv.get(), static_cast<u64>(application_id), static_cast<NcmContentType>(content_type));
                R_SUCCEED();
            }
        }

        /* Always succeed for web applets asking about HBL. */
        /* This enables hbl html. */
        bool is_hbl;
        if (R_SUCCEEDED(pm::info::IsHblProgramId(std::addressof(is_hbl), application_id)) && is_hbl) {
            nswebResolveApplicationContentPath(m_srv.get(), static_cast<u64>(application_id), static_cast<NcmContentType>(content_type));
            R_SUCCEED();
        }
        R_RETURN(nswebResolveApplicationContentPath(m_srv.get(), static_cast<u64>(application_id), static_cast<NcmContentType>(content_type)));
    }

    Result NsDocumentService::GetRunningApplicationProgramId(sf::Out<ncm::ProgramId> out, ncm::ProgramId application_id) {
        R_RETURN(nswebGetRunningApplicationProgramId(m_srv.get(), reinterpret_cast<u64 *>(out.GetPointer()), static_cast<u64>(application_id)));
    }

    Result NsWebMitmService::GetDocumentInterface(sf::Out<sf::SharedPointer<impl::IDocumentInterface>> out) {
        /* Open a document interface. */
        NsDocumentInterface doc;
        R_TRY(nsGetDocumentInterfaceFwd(m_forward_service.get(), std::addressof(doc)));
        const sf::cmif::DomainObjectId target_object_id{serviceGetObjectId(std::addressof(doc.s))};

        out.SetValue(sf::CreateSharedObjectEmplaced<impl::IDocumentInterface, NsDocumentService>(m_client_info, std::make_unique<NsDocumentInterface>(doc)), target_object_id);
        R_SUCCEED();
    }

}
