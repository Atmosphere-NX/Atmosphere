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
#include "htc_htc_service_object.hpp"
#include "htc_htcmisc_manager.hpp"

namespace ams::htc::server {

    namespace {

        static constexpr inline size_t NumServers           = 1;
        static constexpr inline size_t MaxSessions          = 30;
        static constexpr inline sm::ServiceName ServiceName = sm::ServiceName::Encode("htc");

        using ServerOptions = sf::hipc::DefaultServerManagerOptions;
        using ServerManager = sf::hipc::ServerManager<NumServers, ServerOptions, MaxSessions>;

        constinit util::TypedStorage<ServerManager> g_server_manager_storage;
        constinit ServerManager *g_server_manager = nullptr;

        constinit HtcmiscImpl *g_misc_impl = nullptr;

    }

    void InitializeHtcmiscServer(htclow::HtclowManager *htclow_manager) {
        /* Check that we haven't already initialized. */
        AMS_ASSERT(g_server_manager == nullptr);

        /* Create/Set the server manager pointer. */
        g_server_manager = util::ConstructAt(g_server_manager_storage);

        /* Create and register the htc manager object. */
        HtcServiceObject *service_object;
        R_ABORT_UNLESS(g_server_manager->RegisterObjectForServer(CreateHtcmiscManager(std::addressof(service_object), htclow_manager), ServiceName, MaxSessions));

        /* Set the misc impl. */
        g_misc_impl = service_object->GetHtcmiscImpl();

        /* Start the server. */
        g_server_manager->ResumeProcessing();
    }

    void FinalizeHtcmiscServer() {
        /* Check that we've already initialized. */
        AMS_ASSERT(g_server_manager != nullptr);

        /* Clear the misc impl. */
        g_misc_impl = nullptr;

        /* Clear and destroy. */
        std::destroy_at(g_server_manager);
        g_server_manager = nullptr;
    }

    void LoopHtcmiscServer() {
        /* Check that we've already initialized. */
        AMS_ASSERT(g_server_manager != nullptr);

        g_server_manager->LoopProcess();
    }

    void RequestStopHtcmiscServer() {
        /* Check that we've already initialized. */
        AMS_ASSERT(g_server_manager != nullptr);

        g_server_manager->RequestStopProcessing();
    }

    HtcmiscImpl *GetHtcmiscImpl() {
        return g_misc_impl;
    }

}
