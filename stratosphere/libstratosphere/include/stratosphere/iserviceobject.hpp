/*
 * Copyright (c) 2018 Atmosphère-NX
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
 
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
