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

#pragma once
#include <switch.h>

#include "mitm_query_service.hpp"
#include "mitm_session.hpp"
#include "../utilities.hpp"

void RegisterMitmServerQueryHandle(Handle query_h, ServiceObjectHolder &&service);

template <typename T, auto MakeShared>
class MitmServer : public IWaitable {
    static_assert(std::is_base_of<IMitmServiceObject, T>::value, "MitM Service Objects must derive from IMitmServiceObject");
    private:
        Handle port_handle;
        unsigned int max_sessions;
        sts::sm::ServiceName mitm_name;

    public:
        MitmServer(const char *service_name, unsigned int max_s) : port_handle(0), max_sessions(max_s), mitm_name(sts::sm::ServiceName::Encode(service_name)) {
            Handle query_h = INVALID_HANDLE;
            R_ASSERT(sts::sm::mitm::InstallMitm(&this->port_handle, &query_h, this->mitm_name));

            RegisterMitmServerQueryHandle(query_h, std::move(ServiceObjectHolder(std::move(std::make_shared<MitmQueryService<T>>()))));
        }

        virtual ~MitmServer() override {
            if (this->port_handle) {
                R_ASSERT(sts::sm::mitm::UninstallMitm(this->mitm_name));
                svcCloseHandle(port_handle);
            }
        }

        SessionManagerBase *GetSessionManager() {
            return static_cast<SessionManagerBase *>(this->GetManager());
        }

        /* IWaitable */
        virtual Handle GetHandle() override {
            return this->port_handle;
        }

        virtual Result HandleSignaled(u64 timeout) override {
            /* If this server's port was signaled, accept a new session. */
            Handle session_h;
            R_TRY(svcAcceptSession(&session_h, this->port_handle));

            /* Create a forward service for this instance. */
            std::shared_ptr<Service> forward_service(new Service(), [](Service *s) {
                /* Custom deleter to ensure service is open as long as necessary. */
                serviceClose(s);
                delete s;
            });

            u64 client_pid;
            sts::ncm::TitleId client_tid;
            R_ASSERT(sts::sm::mitm::AcknowledgeSession(forward_service.get(), &client_pid, &client_tid, this->mitm_name));

            this->GetSessionManager()->AddWaitable(new MitmSession(session_h, client_pid, forward_service, MakeShared(forward_service, client_pid, client_tid)));
            return ResultSuccess;
        }

};

template<typename T>
struct MakeSharedMitmHelper {
    static constexpr std::shared_ptr<T> Make(std::shared_ptr<Service> forward_srv, u64 client_pid, sts::ncm::TitleId client_tid) {
        return std::make_shared<T>(forward_srv, client_pid, client_tid);
    }
};

template<typename T, auto MakeShared = MakeSharedMitmHelper<T>::Make>
static void AddMitmServerToManager(SessionManagerBase *manager, const char *srv_name, unsigned int max_sessions) {
    auto *srv = new MitmServer<T, MakeShared>(srv_name, max_sessions);
    manager->AddWaitable(srv);
}
