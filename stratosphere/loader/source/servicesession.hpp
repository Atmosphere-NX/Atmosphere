#pragma once
#include <switch.h>
#include <type_traits>

#include "iserviceobject.hpp"
#include "iwaitable.hpp"
#include "serviceserver.hpp"

template <typename T>
class ServiceServer;

template <typename T>
class ServiceSession : public IWaitable {
    static_assert(std::is_base_of<IServiceObject, T>::value, "Service Objects must derive from IServiceObject");
    
    T *service_object;
    ServiceServer<T> *server;
    Handle server_handle;
    Handle client_handle;
    public:
        ServiceSession<T>(ServiceServer<T> *s, Handle s_h, Handle c_h) : server(s), server_handle(s_h), client_handle(c_h) {
            this->service_object = new T();
        }
        
        virtual ~ServiceSession() {
            delete this->service_object;
            if (server_handle) {
                svcCloseHandle(server_handle);
            }
            if (client_handle) {
                svcCloseHandle(client_handle);
            }
        }

        T *get_service_object() { return this->service_object; }
        Handle get_server_handle() { return this->server_handle; }
        Handle get_client_handle() { return this->client_handle; }
        
        /* IWaitable */
        virtual unsigned int get_num_waitables() {
            return 1;
        }
        
        virtual void get_waitables(IWaitable **dst) {
            dst[0] = this;
        }
        
        virtual void delete_child(IWaitable *child) {
            /* TODO: Panic, because we can never have any children. */
        }
        
        virtual Handle get_handle() {
            return this->server_handle;
        }
        
        virtual Result handle_signaled() {
            /* TODO */
            return 0;
        }
};