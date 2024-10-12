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
#pragma once
#include <stratosphere.hpp>

#include "ns_shim.h"

#define AMS_NS_DOCUMENT_MITM_INTERFACE_INFO(C, H)                                                                                                                                                                                                                                                                                          \
    AMS_SF_METHOD_INFO(C, H, 21,   Result, GetApplicationContentPath,      (const sf::OutBuffer &out_path, sf::Out<ams::fs::ContentAttributes> out_attr, ncm::ProgramId application_id, u8 content_type),                                         (out_path, out_attr, application_id, content_type))                                      \
    AMS_SF_METHOD_INFO(C, H, 23,   Result, ResolveApplicationContentPath,  (ncm::ProgramId application_id, u8 content_type),                                                                                                                      (application_id, content_type))                                                          \
    AMS_SF_METHOD_INFO(C, H, 92,   Result, GetRunningApplicationProgramId, (sf::Out<ncm::ProgramId> out, ncm::ProgramId application_id),                                                                                                          (out, application_id),                                              hos::Version_6_0_0)  \
    AMS_SF_METHOD_INFO(C, H, 2524, Result, GetApplicationContentPath2,     (const sf::OutBuffer &out_path, sf::Out<ncm::ProgramId> out_program_id, sf::Out<ams::fs::ContentAttributes> out_attr, ncm::ProgramId application_id, u8 content_type), (out_path, out_program_id, out_attr, application_id, content_type), hos::Version_19_0_0)

AMS_SF_DEFINE_INTERFACE(ams::mitm::ns::impl, IDocumentInterface, AMS_NS_DOCUMENT_MITM_INTERFACE_INFO, 0x0F9B1C00)

#define AMS_NS_WEB_MITM_INTERFACE_INFO(C, H) \
    AMS_SF_METHOD_INFO(C, H, 7999, Result, GetDocumentInterface, (sf::Out<sf::SharedPointer<mitm::ns::impl::IDocumentInterface>> out), (out))

AMS_SF_DEFINE_MITM_INTERFACE(ams::mitm::ns::impl, IWebMitmInterface, AMS_NS_WEB_MITM_INTERFACE_INFO, 0xF4EC2D1A)

namespace ams::mitm::ns {

    class NsDocumentService {
        private:
            sm::MitmProcessInfo m_client_info;
            std::unique_ptr<::NsDocumentInterface> m_srv;
        public:
            NsDocumentService(const sm::MitmProcessInfo &cl, std::unique_ptr<::NsDocumentInterface> s) : m_client_info(cl), m_srv(std::move(s)) { /* .. */ }

            virtual ~NsDocumentService() {
                nsDocumentInterfaceClose(m_srv.get());
            }
        public:
            /* Actual command API. */
            Result GetApplicationContentPath(const sf::OutBuffer &out_path, sf::Out<ams::fs::ContentAttributes> out_attr, ncm::ProgramId application_id, u8 content_type);
            Result ResolveApplicationContentPath(ncm::ProgramId application_id, u8 content_type);
            Result GetRunningApplicationProgramId(sf::Out<ncm::ProgramId> out, ncm::ProgramId application_id);
            Result GetApplicationContentPath2(const sf::OutBuffer &out_path, sf::Out<ncm::ProgramId> out_program_id, sf::Out<ams::fs::ContentAttributes> out_attr, ncm::ProgramId application_id, u8 content_type);
    };
    static_assert(impl::IsIDocumentInterface<NsDocumentService>);

    class NsWebMitmService : public sf::MitmServiceImplBase {
        public:
            using MitmServiceImplBase::MitmServiceImplBase;
        public:
            static bool ShouldMitm(const sm::MitmProcessInfo &client_info) {
                /* We will mitm:
                 * - web applets, to facilitate hbl web browser launching.
                 */
                return ncm::IsWebAppletId(client_info.program_id);
            }
        public:
            Result GetDocumentInterface(sf::Out<sf::SharedPointer<impl::IDocumentInterface>> out);
    };
    static_assert(impl::IsIWebMitmInterface<NsWebMitmService>);

}
