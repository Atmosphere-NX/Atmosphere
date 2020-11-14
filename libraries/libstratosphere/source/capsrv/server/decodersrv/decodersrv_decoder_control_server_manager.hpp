/*
 * Copyright (c) 2019-2020 Atmosphère-NX
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
#include "decodersrv_decoder_control_service.hpp"

namespace ams::capsrv::server {

    class DecoderControlServerManager {
        public:
            /* NOTE: Nintendo only allows one session. */
            static constexpr inline size_t NumServers           = 1;
            static constexpr inline size_t MaxSessions          = 2;
            static constexpr inline sm::ServiceName ServiceName = sm::ServiceName::Encode("caps:dc");

            using Interface     = IDecoderControlService;
            using Service       = DecoderControlService;
            using ServerOptions = sf::hipc::DefaultServerManagerOptions;
            using ServerManager = sf::hipc::ServerManager<NumServers, ServerOptions, MaxSessions>;
        private:
            std::optional<Service> service_holder;
            std::optional<ServerManager> server_manager_holder;
            os::EventType idle_event;
        public:
            constexpr DecoderControlServerManager() : service_holder(), server_manager_holder(), idle_event{} { /* ... */ }

            Result Initialize();
            void   Finalize();

            void StartServer();
            void StopServer();

            void RunServer();
    };

}
