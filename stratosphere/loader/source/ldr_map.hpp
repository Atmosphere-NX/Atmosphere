#pragma once
#include <switch.h>

#include "ldr_registration.hpp"

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
        
        
        static Result MapCodeMemoryForProcessDeprecated(Handle process_h, bool is_64_bit_address_space, u64 base_address, u64 size, u64 *out_code_memory_address);
        static Result MapCodeMemoryForProcessModern(Handle process_h, u64 base_address, u64 size, u64 *out_code_memory_address);
        static Result MapCodeMemoryForProcess(Handle process_h, bool is_64_bit_address_space, u64 base_address, u64 size, u64 *out_code_memory_address);
};

class AutoCloseMap {
    private:
        void *mapped_address;
        u64 base_address;
        u64 size;
        Handle process_handle;
    public:
        AutoCloseMap() : mapped_address(0), base_address(0), size(0), process_handle(0) { };
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
            
            if (R_FAILED((rc = svcMapProcessMemory((void *)try_address, process_h, try_address, size)))) {
                return rc;
            }
            
            this->mapped_address = (void *)try_address;
            this->process_handle = process_h;
            this->base_address = address;
            this->size = size;
            return 0;
        }
        
        void Close() {
            if (this->mapped_address) {
                if (R_FAILED(svcUnmapProcessMemory(this->mapped_address, this->process_handle, this->base_address, this->size))) {
                    /* TODO: panic(). */
                }
            }
        }
};