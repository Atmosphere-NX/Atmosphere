#pragma once
#include <switch.h>
#include <type_traits>

#include "iserviceobject.hpp"
#include "iwaitable.hpp"
#include "servicesession.hpp"

template <typename T>
class ServiceSession;

template <typename T>
class ServiceServer : public IWaitable {
    static_assert(std::is_base_of<IServiceObject, T>::value, "Service Objects must derive from IServiceObject");
    
    Handle port_handle;
    unsigned int max_sessions;
    unsigned int num_sessions;
    ServiceSession<T> **sessions;
    
    public:
        ServiceServer(const char *service_name, unsigned int max_s) {
            if (R_FAILED(smRegisterService(&this->port_handle, service_name, false, this->max_sessions))) {
                /* TODO: Panic. */
            }
            this->sessions = new ServiceSession<T> *[this->max_sessions];
            for (unsigned int i = 0; i < this->max_sessions; i++) {
                this->sessions[i] = NULL;
            }
            this->num_sessions = 0;
        }
        
        virtual ~ServiceServer() {
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
    
        /* IWaitable */
        virtual unsigned int get_num_waitables() {
            unsigned int n = 1;
            for (unsigned int i = 0; i < this->max_sessions; i++) {
                if (this->sessions[i]) {
                    n += this->sessions[i]->get_num_waitables();
                }
            }
            return n;
        }
        
        virtual void get_waitables(IWaitable **dst) {
            dst[0] = this;
            unsigned int n = 0;
            for (unsigned int i = 0; i < this->max_sessions; i++) {
                if (this->sessions[i]) {
                    this->sessions[i]->get_waitables(&dst[1 + n]);
                    n += this->sessions[i]->get_num_waitables();
                }
            }
        }
        
        virtual void delete_child(IWaitable *child) {
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
        
        virtual Handle get_handle() {
            return this->port_handle;
        }
        
        virtual Result handle_signaled() {
            /* If this server's port was signaled, accept a new session. */
            Handle session_h;
            svcAcceptSession(&session_h, this->port_handle);

            if (this->num_sessions >= this->max_sessions) {
                svcCloseHandle(session_h);
                return 0x10601;
            }

            unsigned int i;
            for (i = 0; i < this->max_sessions; i++) {
                if (this->sessions[i] == NULL) {
                    break;
                }
            }

            this->sessions[i] = new ServiceSession<T>(this, session_h, 0);
            this->num_sessions++;
            return 0;
        }
};