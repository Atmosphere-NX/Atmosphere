#pragma once
#include <switch.h>
#include <type_traits>

#include "ipc_templating.hpp"
#include "iserviceobject.hpp"
#include "iwaitable.hpp"
#include "isession.hpp"

template <typename T>
class IPCSession final : public ISession<T> {    
    static_assert(std::is_base_of<IServiceObject, T>::value, "Service Objects must derive from IServiceObject");
    
    public:
        IPCSession<T>(size_t pbs = 0x400) : ISession<T>(NULL, 0, 0, 0) {
            Result rc;
            if (R_FAILED((rc = svcCreateSession(&this->server_handle, &this->client_handle, 0, 0)))) {
                fatalSimple(rc);
            }
            this->service_object = std::make_shared<T>();
            this->pointer_buffer.resize(pbs);
        }
        
        IPCSession<T>(std::shared_ptr<T> so, size_t pbs = 0x400) : ISession<T>(NULL, 0, 0, so, 0) {
            Result rc;
            if (R_FAILED((rc = svcCreateSession(&this->server_handle, &this->client_handle, 0, 0)))) {
                fatalSimple(rc);
            }
            this->pointer_buffer.resize(pbs);
        }
};
