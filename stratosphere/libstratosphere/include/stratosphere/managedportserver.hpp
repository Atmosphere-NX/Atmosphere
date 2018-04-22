#pragma once
#include <switch.h>
#include "iserver.hpp"

template <typename T>
class ManagedPortServer : public IServer<T> {    
    public:
        ManagedPortServer(const char *service_name, unsigned int max_s) : IServer<T>(service_name, max_s) { 
            if (R_FAILED(svcManageNamedPort(&this->port_handle, service_name, this->max_sessions))) {
                /* TODO: panic */
            }
        }
};