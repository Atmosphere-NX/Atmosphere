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
#include <cstdio>

#include "creport_debug_types.hpp"
#include "creport_thread_info.hpp"

struct CodeInfo {
    char name[0x20];
    u8  build_id[0x20];
    u64 start_address;
    u64 end_address;
};

class CodeList {
    public:
        static const size_t max_code_count = 0x60;
        u32 code_count = 0;
        CodeInfo code_infos[max_code_count];
        
        /* For pretty-printing. */
        char address_str_buf[0x280];
    public:        
        void ReadCodeRegionsFromThreadInfo(Handle debug_handle, const ThreadInfo *thread);
        const char *GetFormattedAddressString(u64 address);
        void SaveToFile(FILE *f_report);
    private:
        bool TryFindCodeRegion(Handle debug_handle, u64 guess, u64 *address);
        void AddCodeRegion(u64 debug_handle, u64 code_address);
        void GetCodeInfoName(u64 debug_handle, u64 rx_address, u64 ro_address, char *name);
        void GetCodeInfoBuildId(u64 debug_handle, u64 ro_address, u8 *build_id);
};
