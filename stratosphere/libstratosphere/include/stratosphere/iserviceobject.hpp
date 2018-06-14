#pragma once
#include <switch.h>

template <typename T>
class ISession;

class DomainOwner;

class IServiceObject {
    private:
        DomainOwner *owner = NULL;
    public:
        virtual ~IServiceObject() { }
        
        virtual IServiceObject *clone() = 0;
        
        bool is_domain() { return this->owner != NULL; }
        DomainOwner *get_owner() { return this->owner; }
        void set_owner(DomainOwner *owner) { this->owner = owner; }
        virtual Result dispatch(IpcParsedCommand &r, IpcCommand &out_c, u64 cmd_id, u8 *pointer_buffer, size_t pointer_buffer_size) = 0;
        virtual Result handle_deferred() = 0;
};

#include "domainowner.hpp"
