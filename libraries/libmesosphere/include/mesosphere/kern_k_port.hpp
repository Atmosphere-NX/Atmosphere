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
#include <mesosphere/kern_k_client_port.hpp>
#include <mesosphere/kern_k_server_port.hpp>
#include <mesosphere/kern_slab_helpers.hpp>

namespace ams::kern {

    class KServerSession;
    class KLightServerSession;

    class KPort final : public KAutoObjectWithSlabHeapAndContainer<KPort, KAutoObjectWithList> {
        MESOSPHERE_AUTOOBJECT_TRAITS(KPort, KAutoObject);
        private:
            enum class State : u8 {
                Invalid      = 0,
                Normal       = 1,
                ClientClosed = 2,
                ServerClosed = 3,
            };
        private:
            KServerPort server;
            KClientPort client;
            uintptr_t name;
            State state;
            bool is_light;
        public:
            constexpr KPort() : server(), client(), name(), state(State::Invalid), is_light() { /* ... */ }
            virtual ~KPort() { /* ... */ }

            static void PostDestroy(uintptr_t arg) { MESOSPHERE_UNUSED(arg); /* ... */ }

            void Initialize(s32 max_sessions, bool is_light, uintptr_t name);
            void OnClientClosed();
            void OnServerClosed();

            uintptr_t GetName() const { return this->name; }
            bool IsLight() const { return this->is_light; }

            Result EnqueueSession(KServerSession *session);
            Result EnqueueSession(KLightServerSession *session);

            KClientPort &GetClientPort() { return this->client; }
            KServerPort &GetServerPort() { return this->server; }
            const KClientPort &GetClientPort() const { return this->client; }
            const KServerPort &GetServerPort() const { return this->server; }
    };

}
