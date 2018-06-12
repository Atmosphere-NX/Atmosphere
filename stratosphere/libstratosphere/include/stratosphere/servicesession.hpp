#pragma once
#include <switch.h>
#include <type_traits>

#include "ipc_templating.hpp"
#include "iserviceobject.hpp"
#include "iwaitable.hpp"
#include "iserver.hpp"
#include "isession.hpp"

template <typename T>
class ServiceSession final : public ISession<T> {
    static_assert(std::is_base_of<IServiceObject, T>::value, "Service Objects must derive from IServiceObject");
        
    public:
        ServiceSession<T>(IServer<T> *s, Handle s_h, Handle c_h, size_t pbs = 0x400) : ISession<T>(s, s_h, c_h, pbs) {
            /* ... */
        }
        
};
