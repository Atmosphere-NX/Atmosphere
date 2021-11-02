/*
 * Copyright (c) Atmosphère-NX
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
            s32  path_length;
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
        file.WriteFormat("    Number of Modules:           %zu\n", m_num_modules);
        for (size_t i = 0; i < m_num_modules; i++) {
            const auto& module = m_modules[i];
            file.WriteFormat("    Module %02zu:\n", i);
            file.WriteFormat("        Address:                 %016lx-%016lx\n", module.start_address, module.end_address);
            if (std::strcmp(m_modules[i].name, "") != 0) {
                file.WriteFormat("        Name:                    %s\n", module.name);
            }
            file.DumpMemory("        Module Id:               ", module.module_id, sizeof(module.module_id));
        }
    }

    void ModuleList::FindModulesFromThreadInfo(os::NativeHandle debug_handle, const ThreadInfo &thread, bool is_64_bit) {
        /* Set the debug handle, for access in other member functions. */
        m_debug_handle = debug_handle;

        /* Try to add the thread's PC. */
        this->TryAddModule(thread.GetPC(), is_64_bit);

        /* Try to add the thread's LR. */
        this->TryAddModule(thread.GetLR(), is_64_bit);

        /* Try to add all the addresses in the thread's stacktrace. */
        for (size_t i = 0; i < thread.GetStackTraceSize(); i++) {
            this->TryAddModule(thread.GetStackTrace(i), is_64_bit);
        }
    }

    void ModuleList::TryAddModule(uintptr_t guess, bool is_64_bit) {
        /* Try to locate module from guess. */
        uintptr_t base_address = 0;
        if (!this->TryFindModule(std::addressof(base_address), guess, is_64_bit)) {
            return;
        }

        /* Check whether we already have this module. */
        for (size_t i = 0; i < m_num_modules; i++) {
            if (m_modules[i].start_address <= base_address && base_address < m_modules[i].end_address) {
                return;
            }
        }

        /* Add all contiguous modules. */
        uintptr_t cur_address = base_address;
        while (m_num_modules < ModuleCountMax) {
            /* Get the region extents. */
            svc::MemoryInfo mi;
            svc::PageInfo pi;
            if (R_FAILED(svc::QueryDebugProcessMemory(std::addressof(mi), std::addressof(pi), m_debug_handle, cur_address))) {
                break;
            }

            /* Parse module. */
            if (mi.permission == svc::MemoryPermission_ReadExecute) {
                auto& module = m_modules[m_num_modules++];
                module.start_address = mi.base_address;
                module.end_address   = mi.base_address + mi.size;
                GetModuleName(module.name, module.start_address, module.end_address);
                GetModuleId(module.module_id, module.end_address);

                /* Default to no symbol table. */
                module.has_sym_table = false;

                if (std::strcmp(module.name, "") == 0) {
                    /* Some homebrew won't have a name. Add a fake one for readability. */
                    util::SNPrintf(module.name, sizeof(module.name), "[%02x%02x%02x%02x]", module.module_id[0], module.module_id[1], module.module_id[2], module.module_id[3]);
                } else {
                    /* The module has a name, and so might have a symbol table. Try to add it, if it does. */
                    if (is_64_bit) {
                        DetectModuleSymbolTable(module);
                    }
                }
            }

            /* If we're out of readable memory, we're done reading code. */
            if (mi.state == svc::MemoryState_Free || mi.state == svc::MemoryState_Inaccessible) {
                break;
            }

            /* Verify we're not getting stuck in an infinite loop. */
            if (mi.size == 0 || cur_address + mi.size <= cur_address) {
                break;
            }

            cur_address += mi.size;
        }
    }

    bool ModuleList::TryFindModule(uintptr_t *out_address, uintptr_t guess, bool is_64_bit) {
        AMS_UNUSED(is_64_bit);

        /* Query the memory region our guess falls in. */
        svc::MemoryInfo mi;
        svc::PageInfo pi;
        if (R_FAILED(svc::QueryDebugProcessMemory(std::addressof(mi), std::addressof(pi), m_debug_handle, guess))) {
            return false;
        }

        /* If we fall into a RW region, it may be rwdata. Query the region before it, which may be rodata or text. */
        if (mi.permission == svc::MemoryPermission_ReadWrite) {
            if (R_FAILED(svc::QueryDebugProcessMemory(std::addressof(mi), std::addressof(pi), m_debug_handle, mi.base_address - 4))) {
                return false;
            }
        }

        /* If we fall into an RO region, it may be rodata. Query the region before it, which should be text. */
        if (mi.permission == svc::MemoryPermission_Read) {
            if (R_FAILED(svc::QueryDebugProcessMemory(std::addressof(mi), std::addressof(pi), m_debug_handle, mi.base_address - 4))) {
                return false;
            }
        }

        /* We should, at this point, be looking at an executable region (text). */
        if (mi.permission != svc::MemoryPermission_ReadExecute) {
            return false;
        }

        /* Modules are a series of contiguous (text/rodata/rwdata) regions. */
        /* Iterate backwards until we find unmapped memory, to find the start of the set of modules loaded here. */
        while (mi.base_address > 0) {
            if (R_FAILED(svc::QueryDebugProcessMemory(std::addressof(mi), std::addressof(pi), m_debug_handle, mi.base_address - 4))) {
                return false;
            }

            if (mi.state == svc::MemoryState_Free) {
                /* We've found unmapped memory, so output the mapped memory afterwards. */
                *out_address = mi.base_address + mi.size;
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
            svc::MemoryInfo mi;
            svc::PageInfo pi;

            /* Verify .rodata is read-only. */
            if (R_FAILED(svc::QueryDebugProcessMemory(std::addressof(mi), std::addressof(pi), m_debug_handle, ro_start_address)) || mi.permission != svc::MemoryPermission_Read) {
                return;
            }

            /* Calculate start of rwdata. */
            const u64 rw_start_address = mi.base_address + mi.size;

            /* Read start of .rodata. */
            if (R_FAILED(svc::ReadDebugProcessMemory(reinterpret_cast<uintptr_t>(std::addressof(rodata_start)), m_debug_handle, ro_start_address, sizeof(rodata_start)))) {
                return;
            }

            /* If data is valid under deprecated format, there's no name. */
            if (text_start_address + rodata_start.deprecated_rwdata_offset == rw_start_address) {
                return;
            }

            /* Also validate that we're looking at a valid name. */
            if (rodata_start.module_path.zero != 0 || rodata_start.module_path.path_length <= 0) {
                return;
            }
        }


        /* Start after last slash in path. */
        const char *path = rodata_start.module_path.path;
        int ofs;
        for (ofs = std::min<size_t>(rodata_start.module_path.path_length, sizeof(rodata_start.module_path.path)); ofs >= 0; ofs--) {
            if (path[ofs] == '/' || path[ofs] == '\\') {
                break;
            }
        }
        ofs++;

        /* Copy name to output. */
        const size_t name_size = std::min(ModuleNameLengthMax, std::min<size_t>(rodata_start.module_path.path_length, sizeof(rodata_start.module_path.path)) - ofs);
        std::memcpy(out_name, path + ofs, name_size);
        out_name[ModuleNameLengthMax - 1] = '\x00';
    }

    void ModuleList::GetModuleId(u8 *out, uintptr_t ro_start_address) {
        /* Clear output. */
        std::memset(out, 0, ModuleIdSize);

        /* Verify .rodata is read-only. */
        svc::MemoryInfo mi;
        svc::PageInfo pi;
        if (R_FAILED(svc::QueryDebugProcessMemory(std::addressof(mi), std::addressof(pi), m_debug_handle, ro_start_address)) || mi.permission != svc::MemoryPermission_Read) {
            return;
        }

        /* We want to read the last two pages of .rodata. */
        const size_t read_size = mi.size >= sizeof(g_last_rodata_pages) ? sizeof(g_last_rodata_pages) : (sizeof(g_last_rodata_pages) / 2);
        if (R_FAILED(svc::ReadDebugProcessMemory(reinterpret_cast<uintptr_t>(g_last_rodata_pages), m_debug_handle, mi.base_address + mi.size - read_size, read_size))) {
            return;
        }

        /* Find GNU\x00 to locate start of module id (GNU build id). */
        for (int ofs = read_size - sizeof(GnuSignature) - ModuleIdSize; ofs >= 0; ofs--) {
            if (std::memcmp(g_last_rodata_pages + ofs, GnuSignature, sizeof(GnuSignature)) == 0) {
                std::memcpy(out, g_last_rodata_pages + ofs + sizeof(GnuSignature), ModuleIdSize);
                break;
            }
        }
    }

    void ModuleList::DetectModuleSymbolTable(ModuleInfo &module) {
        /* If we already have a symbol table, no more parsing is needed. */
        if (module.has_sym_table) {
            return;
        }

        /* Declare temporaries. */
        u64 temp_64;
        u32 temp_32;

        /* Get module state. */
        svc::MemoryInfo mi;
        svc::PageInfo pi;
        if (R_FAILED(svc::QueryDebugProcessMemory(std::addressof(mi), std::addressof(pi), m_debug_handle, module.start_address))) {
            return;
        }

        const auto module_state = mi.state;

        /* Verify .rodata is read-only with same state as .text. */
        if (R_FAILED(svc::QueryDebugProcessMemory(std::addressof(mi), std::addressof(pi), m_debug_handle, module.end_address)) || mi.permission != svc::MemoryPermission_Read || mi.state != module_state) {
            return;
        }

        /* Read the first instruction of .text. */
        if (R_FAILED(svc::ReadDebugProcessMemory(reinterpret_cast<uintptr_t>(std::addressof(temp_32)), m_debug_handle, module.start_address, sizeof(temp_32)))) {
            return;
        }

        /* We want to find the symbol table/.dynamic. */
        uintptr_t dyn_address = 0;
        uintptr_t sym_tab     = 0;
        uintptr_t str_tab     = 0;
        size_t    num_sym     = 0;

        /* Detect module type. */
        if (temp_32 == 0) {
            /* Module is dynamically loaded by rtld. */
            u32 mod_offset;
            if (R_FAILED(svc::ReadDebugProcessMemory(reinterpret_cast<uintptr_t>(std::addressof(mod_offset)), m_debug_handle, module.start_address + sizeof(u32), sizeof(u32)))) {
                return;
            }

            if (R_FAILED(svc::ReadDebugProcessMemory(reinterpret_cast<uintptr_t>(std::addressof(temp_32)), m_debug_handle, module.start_address + mod_offset, sizeof(u32)))) {
                return;
            }

            if (temp_32 != rocrt::ModuleHeaderVersion) { /* MOD0 */
                return;
            }

            if (R_FAILED(svc::ReadDebugProcessMemory(reinterpret_cast<uintptr_t>(std::addressof(temp_32)), m_debug_handle, module.start_address + mod_offset + sizeof(u32), sizeof(u32)))) {
                return;
            }

            dyn_address = module.start_address + mod_offset + temp_32;
        } else if (temp_32 == 0x14000002) {
            /* Module embeds rtld. */
            if (R_FAILED(svc::ReadDebugProcessMemory(reinterpret_cast<uintptr_t>(std::addressof(temp_32)), m_debug_handle, module.start_address + 0x5C, sizeof(u32)))) {
                return;
            }

            if (temp_32 != 0x94000002) {
                return;
            }

            if (R_FAILED(svc::ReadDebugProcessMemory(reinterpret_cast<uintptr_t>(std::addressof(temp_32)), m_debug_handle, module.start_address + 0x60, sizeof(u32)))) {
                return;
            }

            dyn_address = module.start_address + 0x60 + temp_32;
        } else {
            /* Module has unknown format. */
            return;
        }


        /* Locate tables inside .dyn. */
        for (size_t ofs = 0; /* ... */; ofs += 0x10) {
            /* Read the DynamicTag. */
            if (R_FAILED(svc::ReadDebugProcessMemory(reinterpret_cast<uintptr_t>(std::addressof(temp_64)), m_debug_handle, dyn_address + ofs, sizeof(u64)))) {
                return;
            }

            if (temp_64 == 0) {
                /* We're done parsing .dyn. */
                break;
            } else if (temp_64 == 4) {
                /* We found DT_HASH */
                if (R_FAILED(svc::ReadDebugProcessMemory(reinterpret_cast<uintptr_t>(std::addressof(temp_64)), m_debug_handle, dyn_address + ofs + sizeof(u64), sizeof(u64)))) {
                    return;
                }

                /* Read nchain, to get the number of symbols. */
                if (R_FAILED(svc::ReadDebugProcessMemory(reinterpret_cast<uintptr_t>(std::addressof(temp_32)), m_debug_handle, module.start_address + temp_64 + sizeof(u32), sizeof(u32)))) {
                    return;
                }

                num_sym = temp_32;
            } else if (temp_64 == 5) {
                /* We found DT_STRTAB */
                if (R_FAILED(svc::ReadDebugProcessMemory(reinterpret_cast<uintptr_t>(std::addressof(temp_64)), m_debug_handle, dyn_address + ofs + sizeof(u64), sizeof(u64)))) {
                    return;
                }

                str_tab = module.start_address + temp_64;
            } else if (temp_64 == 6) {
                /* We found DT_SYMTAB */
                if (R_FAILED(svc::ReadDebugProcessMemory(reinterpret_cast<uintptr_t>(std::addressof(temp_64)), m_debug_handle, dyn_address + ofs + sizeof(u64), sizeof(u64)))) {
                    return;
                }

                sym_tab = module.start_address + temp_64;
            }
        }

        /* Check that we found all the tables. */
        if (!(sym_tab != 0 && str_tab != 0 && num_sym != 0)) {
            return;
        }

        module.has_sym_table = true;
        module.sym_tab       = sym_tab;
        module.str_tab       = str_tab;
        module.num_sym       = static_cast<u32>(num_sym);
    }

    const char *ModuleList::GetFormattedAddressString(uintptr_t address) {
        /* Print default formatted string. */
        util::SNPrintf(m_address_str_buf, sizeof(m_address_str_buf), "%016lx", address);

        /* See if the address is inside a module, for pretty-printing. */
        for (size_t i = 0; i < m_num_modules; i++) {
            const auto& module = m_modules[i];
            if (module.start_address <= address && address < module.end_address) {
                if (module.has_sym_table) {
                    /* Try to locate an appropriate symbol. */
                    for (size_t j = 0; j < module.num_sym; ++j) {
                        /* Read symbol from the module's symbol table. */
                        struct {
                            u32 st_name;
                            u8  st_info;
                            u8  st_other;
                            u16 st_shndx;
                            u64 st_value;
                            u64 st_size;
                        } sym;
                        if (R_FAILED(svc::ReadDebugProcessMemory(reinterpret_cast<uintptr_t>(std::addressof(sym)), m_debug_handle, module.sym_tab + j * sizeof(sym), sizeof(sym)))) {
                            break;
                        }

                        /* Check the symbol is valid/STT_FUNC. */
                        if (sym.st_shndx == 0 || ((sym.st_shndx & 0xFF00) == 0xFF00)) {
                            continue;
                        }
                        if ((sym.st_info & 0xF) != 2) {
                            continue;
                        }

                        /* Check the address. */
                        const uintptr_t func_start = module.start_address + sym.st_value;
                        if (func_start <= address && address < func_start + sym.st_size) {
                            /* Read the symbol name. */
                            const uintptr_t sym_address = module.str_tab + sym.st_name;
                            char sym_name[0x80];
                            if (R_FAILED(svc::ReadDebugProcessMemory(reinterpret_cast<uintptr_t>(sym_name), m_debug_handle, sym_address, sizeof(sym_name)))) {
                                break;
                            }

                            /* Ensure null-termination. */
                            sym_name[sizeof(sym_name) - 1] = '\x00';

                            /* Print the symbol. */
                            util::SNPrintf(m_address_str_buf, sizeof(m_address_str_buf), "%016lx (%s + 0x%lx) (%s + 0x%lx)", address, module.name, address - module.start_address, sym_name, address - func_start);
                            return m_address_str_buf;
                        }
                    }
                }

                util::SNPrintf(m_address_str_buf, sizeof(m_address_str_buf), "%016lx (%s + 0x%lx)", address, module.name, address - module.start_address);
                return m_address_str_buf;
            }
        }

        return m_address_str_buf;
    }

}
