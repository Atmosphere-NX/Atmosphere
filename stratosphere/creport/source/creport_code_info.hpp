#pragma once
#include <switch.h>
#include <cstdio>

#include "creport_debug_types.hpp"

struct CodeInfo {
    char name[0x20];
    u8  build_id[0x20];
    u64 start_address;
    u64 end_address;
};

class CodeList {
    private:
        static const size_t max_code_count = 0x10;
        u32 code_count = 0;
        CodeInfo code_infos[max_code_count];
        
        /* For pretty-printing. */
        char address_str_buf[0x280];
    public:        
        void ReadCodeRegionsFromProcess(Handle debug_handle, u64 pc, u64 lr);
        const char *GetFormattedAddressString(u64 address);
        void SaveToFile(FILE *f_report);
    private:
        bool TryFindCodeRegion(Handle debug_handle, u64 guess, u64 *address);
        void GetCodeInfoName(u64 debug_handle, u64 ro_address, char *name);
        void GetCodeInfoBuildId(u64 debug_handle, u64 ro_address, u8 *build_id);
};
