/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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
#include "sf_hipc_mitm_query_api.hpp"

namespace ams::sf::hipc::impl {

    namespace {

        class MitmQueryService : public IServiceObject {
            private:
                enum class CommandId {
                    ShouldMitm = 65000,
                };
            private:
                ServerManagerBase::MitmQueryFunction query_function;
            public:
                MitmQueryService(ServerManagerBase::MitmQueryFunction qf) : query_function(qf) { /* ... */ }

                void ShouldMitm(sf::Out<bool> out, os::ProcessId process_id, ncm::ProgramId program_id) {
                    out.SetValue(this->query_function(process_id, program_id));
                }
            public:
                DEFINE_SERVICE_DISPATCH_TABLE {
                    MAKE_SERVICE_COMMAND_META(ShouldMitm),
                };
        };

        /* Globals. */
        os::Mutex g_query_server_lock;
        bool g_registered_any = false;

        void QueryServerProcessThreadMain(void *query_server) {
            reinterpret_cast<ServerManagerBase *>(query_server)->LoopProcess();
        }

        constexpr size_t QueryServerProcessThreadStackSize = 0x4000;
        constexpr int    QueryServerProcessThreadPriority  = 27;
        os::StaticThread<QueryServerProcessThreadStackSize> g_query_server_process_thread;

    }

    void RegisterMitmQueryHandle(Handle query_handle, ServerManagerBase::MitmQueryFunction query_func) {
        std::scoped_lock lk(g_query_server_lock);

        constexpr size_t MaxServers = 0;
        sf::hipc::ServerManager<MaxServers> s_query_server;

        R_ASSERT(s_query_server.RegisterSession(query_handle, cmif::ServiceObjectHolder(std::make_shared<MitmQueryService>(query_func))));

        if (!g_registered_any) {
            R_ASSERT(g_query_server_process_thread.Initialize(&QueryServerProcessThreadMain, &s_query_server, QueryServerProcessThreadPriority));
            R_ASSERT(g_query_server_process_thread.Start());
            g_registered_any = true;
        }
    }

}
