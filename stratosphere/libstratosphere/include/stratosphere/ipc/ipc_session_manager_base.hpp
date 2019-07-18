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
#include <atomic>

#include "../waitable_manager_base.hpp"
#include "ipc_service_object.hpp"

class SessionManagerBase : public WaitableManagerBase, public DomainManager {
    public:
        SessionManagerBase() = default;
        virtual ~SessionManagerBase() = default;

        virtual void AddSession(Handle server_h, ServiceObjectHolder &&service) = 0;

        static Result CreateSessionHandles(Handle *server_h, Handle *client_h) {
            return svcCreateSession(server_h, client_h, 0, 0);
        }
};

