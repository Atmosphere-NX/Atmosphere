#pragma once
#include <switch.h>
#include "iserver.hpp"

template <typename T>
class ServiceServer : public IServer<T> {    
    private:
        virtual Result register_self(const char *service_name) {
            return smRegisterService(&this->port_handle, service_name, false, this->max_sessions);
        }
    public:
        ServiceServer(const char *service_name, unsigned int max_s) : IServer<T>(service_name, max_s) { }
};