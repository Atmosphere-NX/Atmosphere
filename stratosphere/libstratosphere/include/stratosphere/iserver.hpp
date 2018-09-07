/*
 * Copyright (c) 2018 Atmosphère-NX
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
#include <algorithm>
#include <type_traits>

#include "iserviceobject.hpp"
#include "iwaitable.hpp"
#include "isession.hpp"

template <typename T>
class ISession;

template <typename T>
class IServer : public IWaitable {
    static_assert(std::is_base_of<IServiceObject, T>::value, "Service Objects must derive from IServiceObject");
    protected:
        Handle port_handle;
        unsigned int max_sessions;
        bool supports_domains;
    
    public:        
        IServer(const char *service_name, unsigned int max_s, bool s_d = false) : max_sessions(max_s), supports_domains(s_d) {
            
        }
        
        virtual ~IServer() {            
            if (port_handle) {
                svcCloseHandle(port_handle);
            }
        }
        
        virtual ISession<T> *get_new_session(Handle session_h) = 0;
    
        /* IWaitable */                        
        virtual Handle get_handle() {
            return this->port_handle;
        }
        
        
        virtual void handle_deferred() {
            /* TODO: Panic, because we can never defer a server. */
        }
        
        virtual Result handle_signaled(u64 timeout) {
            /* If this server's port was signaled, accept a new session. */
            Handle session_h;
            Result rc = svcAcceptSession(&session_h, this->port_handle);
            if (R_FAILED(rc)) {
                return rc;
            }
            
            this->get_manager()->add_waitable(this->get_new_session(session_h));
            return 0;
        }
};
