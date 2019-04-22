/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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
#include <stratosphere.hpp>

class MapUtils { 
    public:
        struct AddressSpaceInfo {
            u64 heap_base;
            u64 heap_size;
            u64 heap_end;
            u64 map_base;
            u64 map_size;
            u64 map_end;
            u64 addspace_base;
            u64 addspace_size;
            u64 addspace_end;
        };
        static Result GetAddressSpaceInfo(AddressSpaceInfo *out, Handle process_h);
        static Result LocateSpaceForMapDeprecated(u64 *out, u64 out_size);
        static Result LocateSpaceForMapModern(u64 *out, u64 out_size);
        static Result LocateSpaceForMap(u64 *out, u64 out_size);
};

class AutoCloseMap {
    private:
        void *mapped_address = nullptr;
        u64 base_address = 0;
        u64 size = 0;
        Handle process_handle = 0;
    public:
        ~AutoCloseMap() {
            Close();
        }
        
        void *GetMappedAddress() {
            return this->mapped_address;
        }
        
        Result Open(Handle process_h, u64 address, u64 size) {
            Result rc;
            u64 try_address;
            if (R_FAILED(rc = MapUtils::LocateSpaceForMap(&try_address, size))) {
                return rc;
            }
            
            if (R_FAILED((rc = svcMapProcessMemory((void *)try_address, process_h, address, size)))) {
                return rc;
            }
            
            this->mapped_address = (void *)try_address;
            this->process_handle = process_h;
            this->base_address = address;
            this->size = size;
            return ResultSuccess;
        }
        
        void Close() {
            if (this->mapped_address) {
                if (R_FAILED(svcUnmapProcessMemory(this->mapped_address, this->process_handle, this->base_address, this->size))) {
                    /* TODO: panic(). */
                }
                this->mapped_address = NULL;
            }
        }
};
