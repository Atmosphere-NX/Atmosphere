#include <switch.h>

#include "servicesession.hpp"

/* IWaitable functions. */
template <typename T>
unsigned int ServiceSession<T>::get_num_waitables() {
    return 1;
}

template <typename T>
void ServiceSession<T>::get_waitables(IWaitable **dst) {
    dst[0] = this;
}

template <typename T>
void ServiceSession<T>::delete_child(IWaitable *child) {
    /* TODO: Panic, because we can never have any children. */
}

template <typename T>
Handle ServiceSession<T>::get_handle() {
    return this->server_handle;
}

template <typename T>
Result ServiceSession<T>::handle_signaled() {
    /* TODO */
    return 0;
}