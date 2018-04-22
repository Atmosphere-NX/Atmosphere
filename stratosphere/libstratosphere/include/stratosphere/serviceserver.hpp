#pragma once
#include <switch.h>
#include "iserver.hpp"

template <typename T>
class ServiceServer : public IServer<T> {    
    public:
        ServiceServer(const char *service_name, unsigned int max_s) : IServer<T>(service_name, max_s) { 
            if (R_FAILED(smRegisterService(&this->port_handle, service_name, false, this->max_sessions))) {
                /* TODO: Panic. */
            }
        }
};