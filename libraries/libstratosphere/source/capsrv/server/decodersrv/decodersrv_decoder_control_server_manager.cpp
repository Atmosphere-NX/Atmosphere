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
#include "decodersrv_decoder_server_object.hpp"

namespace ams::capsrv::server {

    Result DecoderControlServerManager::Initialize() {
        /* Create the objects. */
        this->service_holder.emplace();
        this->server_manager_holder.emplace();

        /* Register the service. */
        R_ABORT_UNLESS((this->server_manager_holder->RegisterServer<Interface, Service>(ServiceName, MaxSessions, sf::GetSharedPointerTo<Interface>(*this->service_holder))));

        /* Initialize the idle event, we're idle initially. */
        os::InitializeEvent(std::addressof(this->idle_event), true, os::EventClearMode_ManualClear);

        return ResultSuccess();
    }

    void DecoderControlServerManager::Finalize() {
        /* Check that the server is idle. */
        AMS_ASSERT(os::TryWaitEvent(std::addressof(this->idle_event)));

        /* Destroy the server. */
        os::FinalizeEvent(std::addressof(this->idle_event));
        this->server_manager_holder = std::nullopt;
        this->service_holder        = std::nullopt;
    }

    void DecoderControlServerManager::StartServer() {
        this->server_manager_holder->ResumeProcessing();
    }

    void DecoderControlServerManager::StopServer() {
        /* Request the server stop, and wait until it does. */
        this->server_manager_holder->RequestStopProcessing();
        os::WaitEvent(std::addressof(this->idle_event));
    }

    void DecoderControlServerManager::RunServer() {
        /* Ensure that we are allowed to run. */
        AMS_ABORT_UNLESS(os::TryWaitEvent(std::addressof(this->idle_event)));

        /* Clear the event. */
        os::ClearEvent(std::addressof(this->idle_event));

        /* Process forever. */
        this->server_manager_holder->LoopProcess();

        /* Signal that we're idle again. */
        os::SignalEvent(std::addressof(this->idle_event));
    }

}
