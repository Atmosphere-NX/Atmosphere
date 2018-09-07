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
#include <atomic>

#include <stratosphere.hpp>

#include "debug.hpp"

class IMitMServiceObject : public IServiceObject {
    protected:
        Service *forward_service;
        u64 process_id = 0;
        u64 title_id = 0;
    public:
        IMitMServiceObject(Service *s) : forward_service(s) {
            
        }
        
        static bool should_mitm(u64 pid, u64 tid) {
            return true;
        }
                
        virtual void clone_to(void *o) = 0;
    protected:
        virtual ~IMitMServiceObject() = default;
        virtual Result dispatch(IpcParsedCommand &r, IpcCommand &out_c, u64 cmd_id, u8 *pointer_buffer, size_t pointer_buffer_size) = 0;
        virtual void postprocess(IpcParsedCommand &r, IpcCommand &out_c, u64 cmd_id, u8 *pointer_buffer, size_t pointer_buffer_size) = 0;
        virtual Result handle_deferred() = 0;
};
