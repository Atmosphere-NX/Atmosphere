#pragma once
#include <switch.h>
#include <stratosphere.hpp>

#include "mitm_query_service.hpp"
#include "sm_mitm.h"
#include "mitm_session.hpp"

#include "debug.hpp"

template <typename T>
class MitMSession;

template <typename T>
class MitMServer final : public IServer<T> {          
    static_assert(std::is_base_of<IServiceObject, T>::value, "Service Objects must derive from IServiceObject");
    private:
        char mitm_name[9];
    
    public:
        MitMServer(ISession<MitMQueryService<T>> **out_query_session, const char *service_name, unsigned int max_s, bool s_d = false) : IServer<T>(service_name, max_s, s_d) {
            Handle tmp_hnd;
            Handle out_query_h;
            Result rc;
            
            if (R_SUCCEEDED((rc = smGetServiceOriginal(&tmp_hnd, smEncodeName(service_name))))) {
                svcCloseHandle(tmp_hnd);
            } else {
                fatalSimple(rc);
            }
            strncpy(mitm_name, service_name, 8);
            mitm_name[8] = '\x00';
            if (R_FAILED((rc = smMitMInstall(&this->port_handle, &out_query_h, mitm_name)))) {
                fatalSimple(rc);           
            }
            *out_query_session = new ServiceSession<MitMQueryService<T>>(NULL, out_query_h, 0);
        }
        
        virtual ~MitMServer() {
            if (this->port_handle) {
                if (R_FAILED(smMitMUninstall(this->mitm_name))) {
                    /* TODO: Panic. */
                }
                /* svcCloseHandle(port_handle); was called by ~IServer. */
            }
        }
        
        ISession<T> *get_new_session(Handle session_h) override {
            return new MitMSession<T>(this, session_h, 0, mitm_name);
        }
};


