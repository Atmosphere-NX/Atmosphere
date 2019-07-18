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

#include "iwaitable.hpp"
#include "ipc.hpp"
#include "utilities.hpp"

template<typename T, auto MakeShared>
class IServer : public IWaitable {
    static_assert(std::is_base_of<IServiceObject, T>::value, "Service Objects must derive from IServiceObject");
    protected:
        Handle port_handle;
        unsigned int max_sessions;

    public:
        IServer(unsigned int max_s) : port_handle(0), max_sessions(max_s) { }

        virtual ~IServer() {
            if (port_handle) {
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

            this->GetSessionManager()->AddSession(session_h, std::move(ServiceObjectHolder(std::move(MakeShared()))));
            return ResultSuccess;
        }
};

template <typename T, auto MakeShared = std::make_shared<T>>
class ServiceServer : public IServer<T, MakeShared> {
    public:
        ServiceServer(const char *service_name, unsigned int max_s) : IServer<T, MakeShared>(max_s) {
            DoWithSmSession([&]() {
                R_ASSERT(smRegisterService(&this->port_handle, service_name, false, this->max_sessions));
            });
        }
};

template <typename T, auto MakeShared = std::make_shared<T>>
class ExistingPortServer : public IServer<T, MakeShared> {
    public:
        ExistingPortServer(Handle port_h, unsigned int max_s) : IServer<T, MakeShared>(max_s) {
            this->port_handle = port_h;
        }
};

template <typename T, auto MakeShared = std::make_shared<T>>
class ManagedPortServer : public IServer<T, MakeShared> {
    public:
        ManagedPortServer(const char *service_name, unsigned int max_s) : IServer<T, MakeShared>(max_s) {
            R_ASSERT(svcManageNamedPort(&this->port_handle, service_name, this->max_sessions));
        }
};