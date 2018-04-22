#pragma once
#include <switch.h>
#include "iserver.hpp"

template <typename T>
class ExistingPortServer : public IServer<T> {    
    private:
        virtual Result register_self(const char *service_name) {
            return 0;
        }
    public:
        ExistingPortServer(Handle port_h, unsigned int max_s) : IServer<T>(NULL, max_s) {
            this->port_handle = port_h;
        }
};