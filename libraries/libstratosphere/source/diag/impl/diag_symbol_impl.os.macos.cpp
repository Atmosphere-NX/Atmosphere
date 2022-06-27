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
#include "diag_symbol_impl.hpp"

#include <unistd.h>
#include <mach-o/dyld.h>
#include <mach-o/loader.h>
#include <mach-o/nlist.h>
#include <mach-o/stab.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/param.h>
#include <sys/mman.h>
#include <cxxabi.h>


extern "C" {

    void __module_offset_helper() { /* ... */ }

}

namespace ams::diag::impl {

    namespace {

        class CurrentExecutableHelper {
            private:
                struct SymbolInfo {
                    uintptr_t address;
                    const char *name;
                };
            private:
                os::NativeHandle m_fd;
                void *m_file_map;
                size_t m_file_size;
                SymbolInfo *m_symbols;
                size_t m_num_symbol;
                const char *m_module_name;
                uintptr_t m_module_address;
                size_t m_module_size;
                uintptr_t m_module_displacement;
            private:
                CurrentExecutableHelper() : m_fd(-1), m_file_map(nullptr), m_file_size(0), m_symbols(nullptr), m_num_symbol(0), m_module_name(nullptr), m_module_address(0), m_module_size(0), m_module_displacement(0) {
                    /* Get the current executable name. */
                    char exe_path[4_KB] = {};
                    GetExecutablePath(exe_path, sizeof(exe_path));

                    /* Open the current executable. */
                    os::NativeHandle fd;
                    do {
                        fd = ::open(exe_path, O_RDONLY);
                    } while (fd < 0 && errno == EINTR);
                    if (fd < 0) {
                        return;
                    }
                    ON_SCOPE_EXIT { if (fd >= 0) { s32 ret; do { ret = ::close(fd); } while (ret < 0 && errno == EINTR); } };

                    /* Get the file size. */
                    struct stat st;
                    if (fstat(fd, std::addressof(st)) < 0) {
                        return;
                    }

                    /* Check that the file can be mapped. */
                    const size_t exe_size = st.st_size;
                    if (exe_size == 0) {
                        return;
                    }

                    /* Map the executable. */
                    void *exe_map = mmap(nullptr, exe_size, PROT_READ, MAP_PRIVATE, fd, 0);
                    if (exe_map == MAP_FAILED) {
                        return;
                    }
                    ON_SCOPE_EXIT { if (exe_map != nullptr) { munmap(exe_map, exe_size); } };

                    /* Get the file's u32 magic. */
                    const uintptr_t exe_start = reinterpret_cast<uintptr_t>(exe_map);
                    const u32 magic = *reinterpret_cast<const u32 *>(exe_start);

                    /* Get/parse the mach header. */
                    u32 ncmds;
                    bool is_64;
                    if (magic == MH_MAGIC) {
                        const auto *header = reinterpret_cast<const struct mach_header *>(exe_start);
                        ncmds       = header->ncmds;
                        is_64       = false;
                    } else if (magic == MH_MAGIC_64) {
                        const auto *header = reinterpret_cast<const struct mach_header_64 *>(exe_start);
                        ncmds       = header->ncmds;
                        is_64       = true;
                    } else {
                        return;
                    }

                    /* Find the symbol load command. */
                    const auto *lc = reinterpret_cast<const struct load_command *>(exe_start + (is_64 ? sizeof(struct mach_header_64) : sizeof(struct mach_header)));
                    for (u32 i = 0; i < ncmds; ++i) {
                        /* If we encounter the symbol table, parse it. */
                        if (lc->cmd == LC_SYMTAB) {
                            if (is_64) {
                                this->ParseSymbolTable<struct nlist_64>(exe_start, reinterpret_cast<const struct symtab_command *>(lc));
                            } else {
                                this->ParseSymbolTable<struct nlist>(exe_start, reinterpret_cast<const struct symtab_command *>(lc));
                            }
                            break;
                        } else if (lc->cmd == LC_SEGMENT) {
                            const auto *sc = reinterpret_cast<const struct segment_command *>(lc);
                            if (std::strcmp(sc->segname, "__TEXT") == 0) {
                                AMS_ASSERT(m_module_address == 0);
                                m_module_address = sc->vmaddr;
                                m_module_size    = sc->vmsize;
                                AMS_ASSERT(m_module_address != 0);
                            }
                        } else if (lc->cmd == LC_SEGMENT_64) {
                            const auto *sc = reinterpret_cast<const struct segment_command_64 *>(lc);
                            if (std::strcmp(sc->segname, "__TEXT") == 0) {
                                AMS_ASSERT(m_module_address == 0);
                                m_module_address = sc->vmaddr;
                                m_module_size    = sc->vmsize;
                                AMS_ASSERT(m_module_address != 0);
                            }
                        }

                        /* Advance to the next load command. */
                        lc = reinterpret_cast<const struct load_command *>(reinterpret_cast<uintptr_t>(lc) + lc->cmdsize);
                    }

                    for (size_t i = 0; i < m_num_symbol; ++i) {
                        if (std::strcmp(m_symbols[i].name, "___module_offset_helper") == 0) {
                            m_module_displacement = reinterpret_cast<uintptr_t>(&__module_offset_helper) - m_symbols[i].address;
                            break;
                        }
                    }

                    if (m_module_address > 0 && m_module_size > 0 && m_num_symbol > 0) {
                        std::swap(m_fd, fd);
                        std::swap(m_file_map, exe_map);
                        m_file_size = exe_size;
                    }
                }

                ~CurrentExecutableHelper() {
                    if (m_file_map != nullptr) {
                        munmap(m_file_map, m_file_size);
                    }

                    if (m_fd >= 0) {
                        s32 ret;
                        do { ret = ::close(m_fd); } while (ret < 0 && errno == EINTR);
                    }
                }
            public:
                static CurrentExecutableHelper &GetInstance() {
                    AMS_FUNCTION_LOCAL_STATIC(CurrentExecutableHelper, s_current_executable_helper_instance);
                    return s_current_executable_helper_instance;
                }
            private:
                template<typename NlistType>
                void ParseSymbolTable(uintptr_t exe_start, const struct symtab_command *c) {
                    /* Check pre-conditions. */
                    AMS_ASSERT(m_fd == -1);
                    AMS_ASSERT(m_file_map == nullptr);
                    AMS_ASSERT(m_symbols == nullptr);

                    /* Get the strtab/symtab. */
                    const auto *symtab = reinterpret_cast<const NlistType *>(exe_start + c->symoff);
                    const char *strtab = reinterpret_cast<const char *>(exe_start + c->stroff);

                    /* Determine the number of functions. */
                    size_t funcs = 0;

                    for (size_t i = 0; i < c->nsyms; ++i) {
                        if (symtab[i].n_type != N_FUN || symtab[i].n_sect == NO_SECT) {
                            continue;
                        }

                        ++funcs;
                    }

                    /* Allocate functions. */
                    m_symbols = reinterpret_cast<SymbolInfo *>(std::malloc(sizeof(SymbolInfo) * funcs));
                    if (m_symbols == nullptr) {
                        return;
                    }

                    /* Set all symbols. */
                    m_num_symbol = 0;
                    for (size_t i = 0; i < c->nsyms; ++i) {
                        if (symtab[i].n_type != N_FUN || symtab[i].n_sect == NO_SECT) {
                            continue;
                        }

                        m_symbols[m_num_symbol].address = symtab[i].n_value;
                        m_symbols[m_num_symbol].name    = strtab + symtab[i].n_un.n_strx;

                        ++m_num_symbol;
                    }
                    AMS_ASSERT(m_num_symbol == funcs);

                    /* Sort the symbols. */
                    std::sort(m_symbols + 0, m_symbols + m_num_symbol, [] (const SymbolInfo &lhs, const SymbolInfo &rhs) {
                        return lhs.address < rhs.address;
                    });
                }

                size_t GetSymbolSizeImpl(const SymbolInfo *symbol) const {
                    /* Do our best to guess. */
                    if (symbol != m_symbols + m_num_symbol - 1) {
                        return (symbol + 1)->address - symbol->address;
                    } else if (m_module_address + m_module_size >= symbol->address) {
                        return m_module_address + m_module_size - symbol->address;
                    } else {
                        return 0;
                    }
                }

                const SymbolInfo *GetBestSymbol(uintptr_t address) const {
                    address -= m_module_displacement;

                    const SymbolInfo *best_symbol = std::lower_bound(m_symbols + 0, m_symbols + m_num_symbol, address, [](const SymbolInfo &lhs, uintptr_t rhs) {
                        return lhs.address < rhs;
                    });

                    if (best_symbol == m_symbols + m_num_symbol) {
                        return nullptr;
                    }

                    if (best_symbol->address != address && best_symbol > m_symbols) {
                        --best_symbol;
                    }

                    const auto vma = best_symbol->address;
                    const auto end = vma + this->GetSymbolSizeImpl(best_symbol);

                    if (vma <= address && address < end) {
                        return best_symbol;
                    } else {
                        return nullptr;
                    }
                }
            public:
                uintptr_t GetSymbolName(char *dst, size_t dst_size, uintptr_t address) const {
                    if (m_fd < 0) {
                        return 0;
                    }

                    /* Get the symbol. */
                    const auto *symbol = this->GetBestSymbol(address);
                    if (symbol == nullptr) {
                        return 0;
                    }

                    /* Print the symbol. */
                    const char *name = symbol->name;

                    int cpp_name_status = 0;
                    if (char *demangled = abi::__cxa_demangle(name, nullptr, 0, std::addressof(cpp_name_status)); cpp_name_status == 0) {
                        AMS_ASSERT(demangled != nullptr);
                        util::TSNPrintf(dst, dst_size, "%s", demangled);
                        std::free(demangled);
                    } else {
                        util::TSNPrintf(dst, dst_size, "%s", name);
                    }

                    return symbol->address + m_module_displacement;
                }

                size_t GetSymbolSize(uintptr_t address) const {
                    if (m_fd < 0) {
                        return 0;
                    }

                    /* Get the symbol. */
                    const auto *symbol = this->GetBestSymbol(address);
                    if (symbol == nullptr) {
                        return 0;
                    }

                    return this->GetSymbolSizeImpl(symbol);
                }
            private:
                static void GetExecutablePath(char *dst, size_t dst_size) {
                    u32 len = dst_size;
                    if (_NSGetExecutablePath(dst, std::addressof(len)) != 0) {
                        dst[0] = 0;
                        return;
                    }
                }
        };

    }

    uintptr_t GetSymbolNameImpl(char *dst, size_t dst_size, uintptr_t address) {
        return CurrentExecutableHelper::GetInstance().GetSymbolName(dst, dst_size, address);
    }

    size_t GetSymbolSizeImpl(uintptr_t address) {
        return CurrentExecutableHelper::GetInstance().GetSymbolSize(address);
    }

}
