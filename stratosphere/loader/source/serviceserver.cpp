#include <switch.h>

#include "serviceserver.hpp"

template <typename T>
ServiceServer<T>::ServiceServer(const char *service_name, unsigned int max_s) : max_sessions(max_s) {
    if (R_FAILED(smRegisterService(&this->port_handle, service_name, false, this->max_sessions))) {
        /* TODO: Panic. */
    }
    this->sessions = new ServiceSession<T> *[this->max_sessions];
    for (unsigned int i = 0; i < this->max_sessions; i++) {
        this->sessions[i] = NULL;
    }
}

template <typename T>
ServiceServer<T>::~ServiceServer()  {
    for (unsigned int i = 0; i < this->max_sessions; i++) {
        if (this->sessions[i]) {
            delete this->sessions[i];
        }
        
        delete this->sessions;
    }
    
    if (port_handle) {
        svcCloseHandle(port_handle);
    }
}

template <typename T>
Result ServiceServer<T>::process() {
    /* TODO */
    return 0;
}