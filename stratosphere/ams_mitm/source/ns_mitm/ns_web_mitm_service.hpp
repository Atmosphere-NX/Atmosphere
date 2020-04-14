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
#include <stratosphere.hpp>

#include "ns_shim.h"

namespace ams::mitm::ns {

    class NsDocumentService : public sf::IServiceObject {
        private:
            enum class CommandId {
                GetApplicationContentPath      = 21,
                ResolveApplicationContentPath  = 23,
                GetRunningApplicationProgramId = 92,
            };
        private:
            sm::MitmProcessInfo client_info;
            std::unique_ptr<::NsDocumentInterface> srv;
        public:
            NsDocumentService(const sm::MitmProcessInfo &cl, std::unique_ptr<::NsDocumentInterface> s) : client_info(cl), srv(std::move(s)) { /* .. */ }

            virtual ~NsDocumentService() {
                nsDocumentInterfaceClose(this->srv.get());
            }
        protected:
            /* Actual command API. */
            Result GetApplicationContentPath(const sf::OutBuffer &out_path, ncm::ProgramId application_id, u8 content_type);
            Result ResolveApplicationContentPath(ncm::ProgramId application_id, u8 content_type);
            Result GetRunningApplicationProgramId(sf::Out<ncm::ProgramId> out, ncm::ProgramId application_id);
        public:
            DEFINE_SERVICE_DISPATCH_TABLE {
                MAKE_SERVICE_COMMAND_META(GetApplicationContentPath),
                MAKE_SERVICE_COMMAND_META(ResolveApplicationContentPath),
                MAKE_SERVICE_COMMAND_META(GetRunningApplicationProgramId, hos::Version_6_0_0),
            };
    };

    class NsWebMitmService  : public sf::IMitmServiceObject {
        private:
            enum class CommandId {
                GetDocumentInterface = 7999,
            };
        public:
            static bool ShouldMitm(const sm::MitmProcessInfo &client_info) {
                /* We will mitm:
                 * - web applets, to facilitate hbl web browser launching.
                 */
                return ncm::IsWebAppletId(client_info.program_id);
            }
        public:
            SF_MITM_SERVICE_OBJECT_CTOR(NsWebMitmService) { /* ... */ }
        protected:
            Result GetDocumentInterface(sf::Out<std::shared_ptr<NsDocumentService>> out);
        public:
            DEFINE_SERVICE_DISPATCH_TABLE {
                MAKE_SERVICE_COMMAND_META(GetDocumentInterface),
            };
    };

}
