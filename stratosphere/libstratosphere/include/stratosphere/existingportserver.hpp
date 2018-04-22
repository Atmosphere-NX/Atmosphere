#pragma once
#include <switch.h>
#include "iserver.hpp"

template <typename T>
class ExistingPortServer : public IServer<T> {    
    public:
        ExistingPortServer(Handle port_h, unsigned int max_s) : IServer<T>(NULL, max_s) {
            this->port_handle = port_h;
        }
};