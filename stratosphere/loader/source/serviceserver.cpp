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
    this->num_sessions = 0;
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

/* IWaitable functions. */
template <typename T>
unsigned int ServiceServer<T>::get_num_waitables() {
    unsigned int n = 1;
    for (unsigned int i = 0; i < this->max_sessions; i++) {
        if (this->sessions[i]) {
            n += this->sessions[i]->get_num_waitables();
        }
    }
    return n;
}

template <typename T>
void ServiceServer<T>::get_waitables(IWaitable **dst) {
    dst[0] = this;
    unsigned int n = 0;
    for (unsigned int i = 0; i < this->max_sessions; i++) {
        if (this->sessions[i]) {
            this->sessions[i]->get_waitables(&dst[1 + n]);
            n += this->sessions[i]->get_num_waitables();
        }
    }
}

template <typename T>
void ServiceSession<T>::delete_child(IWaitable *child) {
    unsigned int i;
    for (i = 0; i < this->max_sessions; i++) {
        if (this->sessions[i] == child) {
            break;
        }
    }
    
    if (i == this->max_sessions) {
        /* TODO: Panic, because this isn't our child. */
    } else {
        delete this->sessions[i];
        this->sessions[i] = NULL;
        this->num_sessions--;
    }
}

template <typename T>
Handle ServiceServer<T>::get_handle() {
    return this->port_handle;
}

template <typename T>
Result ServiceServer<T>::handle_signaled() {
    /* If this server's port was signaled, accept a new session. */
    Handle session_h;
    svcAcceptSession(&session_h, this->port_handle);
    
    if (this->num_sessions >= this->max_sessions) {
        svcCloseHandle(session_h);
        return 0x10601;
    }
    
    unsigned int i;
    for (i = 0; i < this->max_sessions; i++) {
        if (this->sessions[i] = NULL) {
            break;
        }
    }
    
    this->sessions[i] = new ServiceSession<T>(this, session_h, 0);
    this->num_sessions++;
}