#pragma once
#include <switch.h>
#include <type_traits>

#include "iserviceobject.hpp"
#include "servicesession.hpp"

template <typename T>
class ServiceSession;

template <typename T>
class ServiceServer {
    static_assert(std::is_base_of<IServiceObject, T>::value, "Service Objects must derive from IServiceObject");
    
    Handle port_handle;
    unsigned int max_sessions;
    ServiceSession<T> **sessions;
    
    public:
        ServiceServer(const char *service_name, unsigned int max_s);
        ~ServiceServer();
        Result process();
    
};