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
#include <mesosphere/kern_common.hpp>
#include <mesosphere/kern_k_synchronization_object.hpp>
#include <mesosphere/kern_k_server_session.hpp>
#include <mesosphere/kern_k_client_session.hpp>
#include <mesosphere/kern_slab_helpers.hpp>

namespace ams::kern {

    class KClientPort;
    class KProcess;

    class KSession final : public KAutoObjectWithSlabHeapAndContainer<KSession, KAutoObjectWithList> {
        MESOSPHERE_AUTOOBJECT_TRAITS(KSession, KAutoObject);
        private:
            enum class State : u8 {
                Invalid      = 0,
                Normal       = 1,
                ClientClosed = 2,
                ServerClosed = 3,
            };
        private:
            KServerSession m_server;
            KClientSession m_client;
            State m_state;
            KClientPort *m_port;
            uintptr_t m_name;
            KProcess *m_process;
            bool m_initialized;
        public:
            constexpr KSession()
                : m_server(), m_client(), m_state(State::Invalid), m_port(), m_name(), m_process(), m_initialized()
            {
                /* ... */
            }

            virtual ~KSession() { /* ... */ }

            void Initialize(KClientPort *client_port, uintptr_t name);
            virtual void Finalize() override;

            virtual bool IsInitialized() const override { return m_initialized; }
            virtual uintptr_t GetPostDestroyArgument() const override { return reinterpret_cast<uintptr_t>(m_process); }

            static void PostDestroy(uintptr_t arg);

            void OnServerClosed();
            void OnClientClosed();

            bool IsServerClosed() const { return m_state != State::Normal; }
            bool IsClientClosed() const { return m_state != State::Normal; }

            Result OnRequest(KSessionRequest *request) { return m_server.OnRequest(request); }

            KClientSession &GetClientSession() { return m_client; }
            KServerSession &GetServerSession() { return m_server; }
            const KClientSession &GetClientSession() const { return m_client; }
            const KServerSession &GetServerSession() const { return m_server; }

            const KClientPort *GetParent() const { return m_port; }
    };

}
