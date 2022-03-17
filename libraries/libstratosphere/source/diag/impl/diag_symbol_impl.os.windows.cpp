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
#include <stratosphere/windows.hpp>
#include "diag_symbol_impl.hpp"

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
                os::NativeHandle m_file_handle;
                os::NativeHandle m_map_handle;
                void *m_file_map;
                size_t m_file_size;
                SymbolInfo *m_symbols;
                size_t m_num_symbol;
                uintptr_t m_module_address;
                size_t m_module_size;
                uintptr_t m_module_displacement;
            private:
                CurrentExecutableHelper() : m_file_handle(INVALID_HANDLE_VALUE), m_map_handle(INVALID_HANDLE_VALUE), m_file_map(nullptr), m_file_size(0), m_symbols(nullptr), m_num_symbol(0), m_module_address(0), m_module_size(0), m_module_displacement(0) {
                    /* Open the current executable. */
                    auto exe_handle = OpenExecutableFile();
                    if (exe_handle == INVALID_HANDLE_VALUE) {
                        return;
                    }
                    ON_SCOPE_EXIT { if (exe_handle != INVALID_HANDLE_VALUE) { ::CloseHandle(exe_handle); } };

                    /* Get the exe size. */
                    LARGE_INTEGER exe_size_li;
                    if (::GetFileSizeEx(exe_handle, std::addressof(exe_size_li)) == 0) {
                        return;
                    }

                    /* Check the exe size. */
                    s64 exe_size = exe_size_li.QuadPart;
                    if (exe_size == 0) {
                        return;
                    }

                    /* Create a file mapping. */
                    auto map_handle = ::CreateFileMappingW(exe_handle, nullptr, PAGE_READONLY, 0, 0, nullptr);
                    if (map_handle == INVALID_HANDLE_VALUE) {
                        return;
                    }
                    ON_SCOPE_EXIT { if (map_handle != INVALID_HANDLE_VALUE) { ::CloseHandle(map_handle); } };

                    /* Map the file. */
                    void *exe_map = ::MapViewOfFile(map_handle, FILE_MAP_READ, 0, 0, 0);
                    if (exe_map == nullptr) {
                        return;
                    }
                    ON_SCOPE_EXIT { if (exe_map != nullptr) { ::UnmapViewOfFile(exe_map); } };

                    /* Get/parse the DOS header. */
                    const uintptr_t exe_start = reinterpret_cast<uintptr_t>(exe_map);
                    const auto *dos_header = reinterpret_cast<const IMAGE_DOS_HEADER *>(exe_start);
                    if (dos_header->e_magic != IMAGE_DOS_SIGNATURE) {
                        return;
                    }

                    /* Set up the nt headers. */
                    const auto *nt32 = reinterpret_cast<const IMAGE_NT_HEADERS32 *>(exe_start + dos_header->e_lfanew);
                    const auto *nt64 = reinterpret_cast<const IMAGE_NT_HEADERS64 *>(exe_start + dos_header->e_lfanew);
                    if (nt32->FileHeader.Machine == IMAGE_FILE_MACHINE_AMD64 || nt32->FileHeader.Machine == IMAGE_FILE_MACHINE_IA64) {
                        nt32 = nullptr;
                    } else {
                        nt64 = nullptr;
                    }

                    #define EXE_NT_HEADER(x) ((nt64 != nullptr) ? nt64->x : nt32->x)

                    if (EXE_NT_HEADER(Signature) != IMAGE_NT_SIGNATURE) {
                        return;
                    }

                    /* Check that the optional header is really present. */
                    if (EXE_NT_HEADER(FileHeader.SizeOfOptionalHeader) < sizeof(IMAGE_OPTIONAL_HEADER32)) {
                        return;
                    }

                    /* Get sections. */
                    const auto *sec   = nt64 != nullptr ? IMAGE_FIRST_SECTION(nt64) : IMAGE_FIRST_SECTION(nt32);

                    /* Get the symbol table. */
                    const auto *symtab = reinterpret_cast<const IMAGE_SYMBOL *>(exe_start + EXE_NT_HEADER(FileHeader.PointerToSymbolTable));
                    const size_t nsym  = EXE_NT_HEADER(FileHeader.NumberOfSymbols);
                    const char *strtab = reinterpret_cast<const char *>(symtab + nsym);

                    /* Find the section containing our code. */
                    int text_section = -1;
                    for (size_t i = 0; i < nsym; ++i) {
                        if (ISFCN(symtab[i].Type)) {
                            const char *name = symtab[i].N.Name.Short == 0 ? strtab + symtab[i].N.Name.Long : reinterpret_cast<const char *>(symtab[i].N.ShortName);
                            if (std::strcmp(name, "__module_offset_helper") == 0) {
                                AMS_ASSERT(text_section == -1);
                                AMS_ASSERT(m_module_displacement == 0);

                                m_module_displacement = reinterpret_cast<uintptr_t>(&__module_offset_helper) - symtab[i].Value;
                                text_section          = symtab[i].SectionNumber;

                                AMS_ASSERT(m_module_displacement != 0);
                                AMS_ASSERT(text_section != -1);
                                break;
                            }
                        }
                    }

                    /* Determine the number of functions. */
                    size_t funcs = 0;
                    for (size_t i = 0; i < nsym; ++i) {
                        if (ISFCN(symtab[i].Type) && symtab[i].SectionNumber == text_section) {
                            ++funcs;
                        }
                    }

                    /* Allocate functions. */
                    m_symbols = reinterpret_cast<SymbolInfo *>(std::malloc(sizeof(SymbolInfo) * funcs));
                    if (m_symbols == nullptr) {
                        return;
                    }

                    /* Set all symbols. */
                    m_num_symbol = 0;
                    for (size_t i = 0; i < nsym; ++i) {
                        if (ISFCN(symtab[i].Type) && symtab[i].SectionNumber == text_section) {
                            m_symbols[m_num_symbol].address = symtab[i].Value;
                            m_symbols[m_num_symbol].name    = symtab[i].N.Name.Short == 0 ? strtab + symtab[i].N.Name.Long : reinterpret_cast<const char *>(symtab[i].N.ShortName);

                            ++m_num_symbol;
                        }
                    }
                    AMS_ASSERT(m_num_symbol == funcs);

                    /* Sort the symbols. */
                    std::sort(m_symbols + 0, m_symbols + m_num_symbol, [] (const SymbolInfo &lhs, const SymbolInfo &rhs) {
                        return lhs.address < rhs.address;
                    });

                    m_module_address = 0;
                    m_module_size    = sec[text_section - 1].Misc.VirtualSize;

                    if (m_module_displacement != 0 && m_module_size > 0 && m_num_symbol > 0) {
                        std::swap(m_file_handle, exe_handle);
                        std::swap(m_map_handle, map_handle);
                        std::swap(m_file_map, exe_map);
                        m_file_size = exe_size;
                    }
                }

                ~CurrentExecutableHelper() {
                    if (m_file_map != nullptr) {
                        ::UnmapViewOfFile(m_file_map);
                    }
                    if (m_map_handle != nullptr) {
                        ::CloseHandle(m_map_handle);
                    }
                    if (m_file_handle != nullptr) {
                        ::CloseHandle(m_file_handle);
                    }
                }
            public:
                static CurrentExecutableHelper &GetInstance() {
                    AMS_FUNCTION_LOCAL_STATIC(CurrentExecutableHelper, s_current_executable_helper_instance);
                    return s_current_executable_helper_instance;
                }
            private:
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
                    if (m_file_handle == INVALID_HANDLE_VALUE) {
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
                    if (m_file_handle == INVALID_HANDLE_VALUE) {
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
                static os::NativeHandle OpenExecutableFile() {
                    /* Get the module file name. */
                    wchar_t module_file_name[0x1000];
                    if (::GetModuleFileNameW(0, module_file_name, util::size(module_file_name)) == 0) {
                        return INVALID_HANDLE_VALUE;
                    }

                    return ::CreateFileW(module_file_name, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
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
