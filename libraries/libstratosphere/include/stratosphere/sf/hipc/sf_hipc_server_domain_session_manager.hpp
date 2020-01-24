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
#include "sf_hipc_server_session_manager.hpp"
#include "../cmif/sf_cmif_domain_manager.hpp"

namespace ams::sf::hipc {

    class ServerDomainSessionManager : public ServerSessionManager, private cmif::ServerDomainManager {
        protected:
            using cmif::ServerDomainManager::DomainEntryStorage;
            using cmif::ServerDomainManager::DomainStorage;
        protected:
            virtual Result DispatchManagerRequest(ServerSession *session, const cmif::PointerAndSize &in_message, const cmif::PointerAndSize &out_message) override final;
        public:
            ServerDomainSessionManager(DomainEntryStorage *entry_storage, size_t entry_count) : ServerDomainManager(entry_storage, entry_count) { /* ... */ }

            inline cmif::DomainServiceObject *AllocateDomainServiceObject() {
                return cmif::ServerDomainManager::AllocateDomainServiceObject();
            }
    };

}
