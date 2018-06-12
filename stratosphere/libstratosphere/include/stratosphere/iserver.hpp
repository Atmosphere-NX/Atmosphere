#pragma once
#include <switch.h>
#include <type_traits>

#include "iserviceobject.hpp"
#include "iwaitable.hpp"
#include "isession.hpp"

template <typename T>
class ISession;

template <typename T>
class IServer : public IWaitable {
    static_assert(std::is_base_of<IServiceObject, T>::value, "Service Objects must derive from IServiceObject");
    protected:
        Handle port_handle;
        unsigned int max_sessions;
        bool supports_domains;
    
    public:        
        IServer(const char *service_name, unsigned int max_s, bool s_d = false) : max_sessions(max_s), supports_domains(s_d) {
            
        }
        
        virtual ~IServer() {            
            if (port_handle) {
                svcCloseHandle(port_handle);
            }
        }
        
        virtual ISession<T> *get_new_session(Handle session_h) = 0;
    
        /* IWaitable */                        
        virtual Handle get_handle() {
            return this->port_handle;
        }
        
        
        virtual void handle_deferred() {
            /* TODO: Panic, because we can never defer a server. */
        }
        
        virtual Result handle_signaled(u64 timeout) {
            /* If this server's port was signaled, accept a new session. */
            Handle session_h;
            Result rc = svcAcceptSession(&session_h, this->port_handle);
            if (R_FAILED(rc)) {
                return rc;
            }
            
            this->get_manager()->add_waitable(this->get_new_session(session_h));
            return 0;
        }
};