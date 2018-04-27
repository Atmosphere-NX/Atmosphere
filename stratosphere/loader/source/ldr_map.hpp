#pragma once
#include <switch.h>

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
                this->mapped_address = NULL;
            }
        }
};

struct MappedCodeMemory {
    Handle process_handle;
    u64 base_address;
    u64 size;
    u64 code_memory_address;
    void *mapped_address;
    
    bool IsActive() {
        return this->code_memory_address != 0;
    }
    
    bool IsMapped() {
        return this->mapped_address != NULL;
    }
    
    /* Utility functions. */
    Result Open(Handle process_h, bool is_64_bit_address_space, u64 address, u64 size) {
        Result rc;
        if (this->IsActive()) {
            return 0x19009;
        }
        
        this->process_handle = process_h;
        this->base_address = address;
        this->size = size;
                
        if (R_FAILED((rc = MapUtils::MapCodeMemoryForProcess(this->process_handle, is_64_bit_address_space, this->base_address, this->size, &this->code_memory_address)))) {
            Close();
        }   
        return rc;
    }
    
    Result OpenAtAddress(Handle process_h, u64 address, u64 size, u64 target_code_memory_address) {
        Result rc;
        if (this->IsActive()) {
            return 0x19009;
        }
        this->process_handle = process_h;
        this->base_address = address;
        this->size = size;
        
        if (R_SUCCEEDED((rc = svcMapProcessCodeMemory(this->process_handle, target_code_memory_address, this->base_address, this->size)))) {
            this->code_memory_address = target_code_memory_address;
        } else {
            Close();
        }
        return rc;
    }
    
    Result Map() {
        Result rc;
        u64 try_address;
        if (this->IsMapped()) {
            return 0x19009;
        }
        if (R_FAILED(rc = MapUtils::LocateSpaceForMap(&try_address, size))) {
            return rc;
        }
        
        if (R_FAILED((rc = svcMapProcessMemory((void *)try_address, this->process_handle, try_address, size)))) {
            return rc;
        }
        
        this->mapped_address = (void *)try_address;   
        return rc;
    }
    
    void Unmap() {
        if (this->IsMapped()) {
            if (R_FAILED(svcUnmapProcessMemory(this->mapped_address, this->process_handle, this->base_address, this->size))) {
                /* TODO: panic(). */
            }
        }
        this->mapped_address = NULL;
    }
    
    void Close() {
        Unmap();
        if (this->IsActive()) {
            if (R_FAILED(svcUnmapProcessCodeMemory(this->process_handle, this->code_memory_address, this->base_address, this->size))) {
                /* TODO: panic(). */
            }
        }
        *this = (const MappedCodeMemory){0};
    }
};