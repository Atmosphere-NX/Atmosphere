#pragma once
#include <switch.h>
#include "iserver.hpp"

template <typename T>
class ManagedPortServer : public IServer<T> {    
    private:
        virtual Result register_self(const char *service_name) {
            return svcManageNamedPort(&this->port_handle, service_name, this->max_sessions);
        }
    public:
        ManagedPortServer(const char *service_name, unsigned int max_s) : IServer<T>(service_name, max_s) { }
};