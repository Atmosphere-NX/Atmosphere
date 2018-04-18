#pragma once
#include <switch.h>
#include <type_traits>

#include "iserviceobject.hpp"
#include "iwaitable.hpp"
#include "servicesession.hpp"

template <typename T>
class ServiceSession;

template <typename T>
class ServiceServer : IWaitable {
    static_assert(std::is_base_of<IServiceObject, T>::value, "Service Objects must derive from IServiceObject");
    
    Handle port_handle;
    unsigned int max_sessions;
    unsigned int num_sessions;
    ServiceSession<T> **sessions;
    
    public:
        ServiceServer(const char *service_name, unsigned int max_s);
        virtual ~ServiceServer();
    
        /* IWaitable */
        virtual unsigned int get_num_waitables();
        virtual void get_waitables(IWaitable **dst);
        virtual void delete_child(IWaitable *child);
        virtual Handle get_handle();
        virtual Result handle_signaled();
};