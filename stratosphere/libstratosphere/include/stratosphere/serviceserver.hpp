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
#include "iserver.hpp"

template <typename T>
class ServiceServer : public IServer<T> {    
    public:
        ServiceServer(const char *service_name, unsigned int max_s, bool s_d = false) : IServer<T>(service_name, max_s, s_d) { 
            if (R_FAILED(smRegisterService(&this->port_handle, service_name, false, this->max_sessions))) {
                /* TODO: Panic. */
            }
        }
        
        ISession<T> *get_new_session(Handle session_h) override {
            return new ServiceSession<T>(this, session_h, 0);
        }
};