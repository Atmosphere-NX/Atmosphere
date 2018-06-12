#pragma once
#include <switch.h>
#include "iserver.hpp"

template <typename T>
class ManagedPortServer : public IServer<T> {    
    public:
        ManagedPortServer(const char *service_name, unsigned int max_s, bool s_d = false) : IServer<T>(service_name, max_s, s_d) { 
            if (R_FAILED(svcManageNamedPort(&this->port_handle, service_name, this->max_sessions))) {
                /* TODO: panic */
            }
        }
        
        ISession<T> *get_new_session(Handle session_h) override {
            return new ServiceSession<T>(this, session_h, 0);
        }
};