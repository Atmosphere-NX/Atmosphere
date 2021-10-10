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
#include "decodersrv_decoder_server_object.hpp"

namespace ams::capsrv::server {

    Result DecoderControlServerManager::Initialize() {
        /* Create the objects. */
        m_service_holder.emplace();
        m_server_manager_holder.emplace();

        /* Register the service. */
        R_ABORT_UNLESS((m_server_manager_holder->RegisterObjectForServer(m_service_holder->GetShared(), ServiceName, MaxSessions)));

        /* Initialize the idle event, we're idle initially. */
        os::InitializeEvent(std::addressof(m_idle_event), true, os::EventClearMode_ManualClear);

        return ResultSuccess();
    }

    void DecoderControlServerManager::Finalize() {
        /* Check that the server is idle. */
        AMS_ASSERT(os::TryWaitEvent(std::addressof(m_idle_event)));

        /* Destroy the server. */
        os::FinalizeEvent(std::addressof(m_idle_event));
        m_server_manager_holder = util::nullopt;
        m_service_holder        = util::nullopt;
    }

    void DecoderControlServerManager::StartServer() {
        m_server_manager_holder->ResumeProcessing();
    }

    void DecoderControlServerManager::StopServer() {
        /* Request the server stop, and wait until it does. */
        m_server_manager_holder->RequestStopProcessing();
        os::WaitEvent(std::addressof(m_idle_event));
    }

    void DecoderControlServerManager::RunServer() {
        /* Ensure that we are allowed to run. */
        AMS_ABORT_UNLESS(os::TryWaitEvent(std::addressof(m_idle_event)));

        /* Clear the event. */
        os::ClearEvent(std::addressof(m_idle_event));

        /* Process forever. */
        m_server_manager_holder->LoopProcess();

        /* Signal that we're idle again. */
        os::SignalEvent(std::addressof(m_idle_event));
    }

}
