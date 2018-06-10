#pragma once
#include <switch.h>
#include <stratosphere.hpp>

#include "sm_mitm.h"
#include "mitm_session.hpp"

#include "debug.hpp"

template <typename T>
class MitMSession;

template <typename T>
class MitMServer final : public IWaitable {          
    static_assert(std::is_base_of<IServiceObject, T>::value, "Service Objects must derive from IServiceObject");
    protected:
        Handle port_handle;
        unsigned int max_sessions;
        unsigned int num_sessions;
        MitMSession<T> **sessions;
        char mitm_name[9];
    
    public:        
        MitMServer(const char *service_name, unsigned int max_s) : max_sessions(max_s) {
            this->sessions = new MitMSession<T> *[this->max_sessions];
            for (unsigned int i = 0; i < this->max_sessions; i++) {
                this->sessions[i] = NULL;
            }
            this->num_sessions = 0;
            strncpy(mitm_name, service_name, 8);
            mitm_name[8] = '\x00';
            Result rc;
            if (R_FAILED((rc = smMitMInstall(&this->port_handle, mitm_name)))) {
                /* TODO: Panic. */            
            }
        }
        
        virtual ~MitMServer() {
            for (unsigned int i = 0; i < this->max_sessions; i++) {
                if (this->sessions[i]) {
                    delete this->sessions[i];
                }
                
                delete this->sessions;
            }
            
            if (port_handle) {
                if (R_FAILED(smMitMUninstall(mitm_name))) {
                    /* TODO: Panic. */
                }
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
        
        
        virtual void handle_deferred() {
            /* TODO: Panic, because we can never defer a server. */
        }
        
        virtual Result handle_signaled(u64 timeout) {
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

            this->sessions[i] = new MitMSession<T>(this, session_h, 0, mitm_name);
            this->sessions[i]->set_parent(this);
            this->num_sessions++;
            return 0;
        }
};


