/*
 * Copyright (c) 2018-2020 Atmosphère-NX
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
#include <stratosphere.hpp>
#include "creport_modules.hpp"
#include "creport_utils.hpp"

namespace ams::creport {

    namespace {

        /* Convenience definitions/types. */
        constexpr size_t ModulePathLengthMax = 0x200;
        constexpr u8 GnuSignature[4] = {'G', 'N', 'U', 0};

        struct ModulePath {
            u32  zero;
            u32  path_length;
            char path[ModulePathLengthMax];
        };
        static_assert(sizeof(ModulePath) == 0x208, "ModulePath definition!");

        struct RoDataStart {
            union {
                u64 deprecated_rwdata_offset;
                ModulePath module_path;
            };
        };
        static_assert(sizeof(RoDataStart) == sizeof(ModulePath), "RoDataStart definition!");

        /* Globals. */
        u8 g_last_rodata_pages[2 * os::MemoryPageSize];

    }

    void ModuleList::SaveToFile(ScopedFile &file) {
        file.WriteFormat("    Number of Modules:           %zu\n", this->num_modules);
        for (size_t i = 0; i < this->num_modules; i++) {
            const auto& module = this->modules[i];
            file.WriteFormat("    Module %02zu:\n", i);
            file.WriteFormat("        Address:                 %016lx-%016lx\n", module.start_address, module.end_address);
            if (std::strcmp(this->modules[i].name, "") != 0) {
                file.WriteFormat("        Name:                    %s\n", module.name);
            }
            file.DumpMemory("        Build Id:                ", &module.build_id[0], sizeof(module.build_id));
        }
    }

    void ModuleList::FindModulesFromThreadInfo(Handle debug_handle, const ThreadInfo &thread) {
        /* Set the debug handle, for access in other member functions. */
        this->debug_handle = debug_handle;

        /* Try to add the thread's PC. */
        this->TryAddModule(thread.GetPC());

        /* Try to add the thread's LR. */
        this->TryAddModule(thread.GetLR());

        /* Try to add all the addresses in the thread's stacktrace. */
        for (size_t i = 0; i < thread.GetStackTraceSize(); i++) {
            this->TryAddModule(thread.GetStackTrace(i));
        }
    }

    void ModuleList::TryAddModule(uintptr_t guess) {
        /* Try to locate module from guess. */
        uintptr_t base_address = 0;
        if (!this->TryFindModule(&base_address, guess)) {
            return;
        }

        /* Check whether we already have this module. */
        for (size_t i = 0; i < this->num_modules; i++) {
            if (this->modules[i].start_address <= base_address && base_address < this->modules[i].end_address) {
                return;
            }
        }

        /* Add all contiguous modules. */
        uintptr_t cur_address = base_address;
        while (this->num_modules < ModuleCountMax) {
            /* Get the region extents. */
            MemoryInfo mi;
            u32 pi;
            if (R_FAILED(svcQueryDebugProcessMemory(&mi, &pi, this->debug_handle, cur_address))) {
                break;
            }

            /* Parse module. */
            if (mi.perm == Perm_Rx) {
                auto& module = this->modules[this->num_modules++];
                module.start_address = mi.addr;
                module.end_address   = mi.addr + mi.size;
                GetModuleName(module.name, module.start_address, module.end_address);
                GetModuleBuildId(module.build_id, module.end_address);
                /* Some homebrew won't have a name. Add a fake one for readability. */
                if (std::strcmp(module.name, "") == 0) {
                    std::snprintf(module.name, sizeof(module.name), "[%02x%02x%02x%02x]", module.build_id[0], module.build_id[1], module.build_id[2], module.build_id[3]);
                }
            }

            /* If we're out of readable memory, we're done reading code. */
            if (mi.type == MemType_Unmapped || mi.type == MemType_Reserved) {
                break;
            }

            /* Verify we're not getting stuck in an infinite loop. */
            if (mi.size == 0 || cur_address + mi.size <= cur_address) {
                break;
            }

            cur_address += mi.size;
        }
    }

    bool ModuleList::TryFindModule(uintptr_t *out_address, uintptr_t guess) {
        /* Query the memory region our guess falls in. */
        MemoryInfo mi;
        u32 pi;
        if (R_FAILED(svcQueryDebugProcessMemory(&mi, &pi, this->debug_handle, guess))) {
            return false;
        }

        /* If we fall into a RW region, it may be rwdata. Query the region before it, which may be rodata or text. */
        if (mi.perm == Perm_Rw) {
            if (R_FAILED(svcQueryDebugProcessMemory(&mi, &pi, debug_handle, mi.addr - 4))) {
                return false;
            }
        }

        /* If we fall into an RO region, it may be rodata. Query the region before it, which should be text. */
        if (mi.perm == Perm_R) {
            if (R_FAILED(svcQueryDebugProcessMemory(&mi, &pi, debug_handle, mi.addr - 4))) {
                return false;
            }
        }

        /* We should, at this point, be looking at an executable region (text). */
        if (mi.perm != Perm_Rx) {
            return false;
        }

        /* Modules are a series of contiguous (text/rodata/rwdata) regions. */
        /* Iterate backwards until we find unmapped memory, to find the start of the set of modules loaded here. */
        while (mi.addr > 0) {
            if (R_FAILED(svcQueryDebugProcessMemory(&mi, &pi, debug_handle, mi.addr - 4))) {
                return false;
            }

            if (mi.type == MemType_Unmapped) {
                /* We've found unmapped memory, so output the mapped memory afterwards. */
                *out_address = mi.addr + mi.size;
                return true;
            }
        }

        /* Something weird happened here. */
        return false;
    }

    void ModuleList::GetModuleName(char *out_name, uintptr_t text_start_address, uintptr_t ro_start_address) {
        /* Clear output. */
        std::memset(out_name, 0, ModuleNameLengthMax);

        /* Read module path from process memory. */
        RoDataStart rodata_start;
        {
            MemoryInfo mi;
            u32 pi;

            /* Verify .rodata is read-only. */
            if (R_FAILED(svcQueryDebugProcessMemory(&mi, &pi, this->debug_handle, ro_start_address)) || mi.perm != Perm_R) {
                return;
            }

            /* Calculate start of rwdata. */
            const u64 rw_start_address = mi.addr + mi.size;

            /* Read start of .rodata. */
            if (R_FAILED(svcReadDebugProcessMemory(&rodata_start, this->debug_handle, ro_start_address, sizeof(rodata_start)))) {
                return;
            }

            /* If data is valid under deprecated format, there's no name. */
            if (text_start_address + rodata_start.deprecated_rwdata_offset == rw_start_address) {
                return;
            }

            /* Also validate that we're looking at a valid name. */
            if (rodata_start.module_path.zero != 0 || rodata_start.module_path.path_length != strnlen(rodata_start.module_path.path, sizeof(rodata_start.module_path.path))) {
                return;
            }
        }


        /* Start after last slash in path. */
        const char *path = rodata_start.module_path.path;
        int ofs;
        for (ofs = rodata_start.module_path.path_length; ofs >= 0; ofs--) {
            if (path[ofs] == '/' || path[ofs] == '\\') {
                break;
            }
        }
        ofs++;

        /* Copy name to output. */
        const size_t name_size = std::min(ModuleNameLengthMax, sizeof(rodata_start.module_path.path) - ofs);
        std::strncpy(out_name, path + ofs, name_size);
        out_name[ModuleNameLengthMax - 1] = '\x00';
    }

    void ModuleList::GetModuleBuildId(u8 *out_build_id, uintptr_t ro_start_address) {
        /* Clear output. */
        std::memset(out_build_id, 0, ModuleBuildIdLength);

        /* Verify .rodata is read-only. */
        MemoryInfo mi;
        u32 pi;
        if (R_FAILED(svcQueryDebugProcessMemory(&mi, &pi, this->debug_handle, ro_start_address)) || mi.perm != Perm_R) {
            return;
        }

        /* We want to read the last two pages of .rodata. */
        const size_t read_size = mi.size >= sizeof(g_last_rodata_pages) ? sizeof(g_last_rodata_pages) : (sizeof(g_last_rodata_pages) / 2);
        if (R_FAILED(svcReadDebugProcessMemory(g_last_rodata_pages, this->debug_handle, mi.addr + mi.size - read_size, read_size))) {
            return;
        }

        /* Find GNU\x00 to locate start of build id. */
        for (int ofs = read_size - sizeof(GnuSignature) - ModuleBuildIdLength; ofs >= 0; ofs--) {
            if (std::memcmp(g_last_rodata_pages + ofs, GnuSignature, sizeof(GnuSignature)) == 0) {
                std::memcpy(out_build_id, g_last_rodata_pages + ofs + sizeof(GnuSignature), ModuleBuildIdLength);
                break;
            }
        }
    }

    const char *ModuleList::GetFormattedAddressString(uintptr_t address) {
        /* Print default formatted string. */
        std::snprintf(this->address_str_buf, sizeof(this->address_str_buf), "%016lx", address);

        /* See if the address is inside a module, for pretty-printing. */
        for (size_t i = 0; i < this->num_modules; i++) {
            const auto& module = this->modules[i];
            if (module.start_address <= address && address < module.end_address) {
                std::snprintf(this->address_str_buf, sizeof(this->address_str_buf), "%016lx (%s + 0x%lx)", address, module.name, address - module.start_address);
                break;
            }
        }

        return this->address_str_buf;
    }

}
