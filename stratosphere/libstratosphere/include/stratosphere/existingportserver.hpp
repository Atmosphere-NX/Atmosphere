#pragma once
#include <switch.h>
#include "iserver.hpp"

template <typename T>
class ExistingPortServer : public IServer<T> {    
    public:
        ExistingPortServer(Handle port_h, unsigned int max_s, bool s_d = false) : IServer<T>(NULL, max_s, s_d) {
            this->port_handle = port_h;
        }
        
        ISession<T> *get_new_session(Handle session_h) override {
            return new ServiceSession<T>(this, session_h, 0);
        }
};