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

#define PACKAGE "stratosphere"
#define PACKAGE_VERSION STRINGIFY(ATMOSPHERE_RELEASE_VERSION_MAJOR.ATMOSPHERE_RELEASE_VERSION_MINOR.ATMOSPHERE_RELEASE_VERSION_MICRO)

#if defined(ATMOSPHERE_OS_LINUX)
#include <bfd.h>
#include <unistd.h>
#include <dlfcn.h>
#include <link.h>

extern "C" char __init_array_start;
#endif

#include <cxxabi.h>

namespace ams::diag::impl {

    namespace {

        class BfdHelper {
            private:
                bfd *m_handle;
                asymbol **m_symbol;
                size_t m_num_symbol;
                size_t m_num_func_symbol;
                const char *m_module_name;
                uintptr_t m_module_address;
                size_t m_module_size;
            private:
                BfdHelper() : m_handle(nullptr), m_symbol(nullptr), m_module_name(nullptr) {
                    /* Get the current executable name. */
                    char exe_path[4_KB] = {};
                    GetExecutablePath(exe_path, sizeof(exe_path));

                    /* Open bfd. */
                    bfd *b = ::bfd_openr(exe_path, 0);
                    if (b == nullptr) {
                        return;
                    }
                    auto bfd_guard = SCOPE_GUARD { ::bfd_close(b); };

                    /* Check the format. */
                    if (!::bfd_check_format(b, bfd_object)) {
                        return;
                    }

                    /* Verify the file has symbols. */
                    if ((bfd_get_file_flags(b) & HAS_SYMS) == 0) {
                        return;
                    }

                    /* Read the symbols. */
                    unsigned int _;
                    void *symbol_table;
                    s64 num_symbols = bfd_read_minisymbols(b, false, std::addressof(symbol_table), std::addressof(_));
                    if (num_symbols == 0) {
                        num_symbols = bfd_read_minisymbols(b, true, std::addressof(symbol_table), std::addressof(_));
                        if (num_symbols < 0) {
                            return;
                        }
                    }

                    /* We successfully got the symbol table. */
                    bfd_guard.Cancel();

                    m_handle     = b;
                    m_symbol     = reinterpret_cast<asymbol **>(symbol_table);
                    m_num_symbol = static_cast<size_t>(num_symbols);

                    /* Sort the symbol table. */
                    std::sort(m_symbol + 0, m_symbol + m_num_symbol, [] (asymbol *lhs, asymbol *rhs) {
                        const bool l_func = (lhs->flags & BSF_FUNCTION);
                        const bool r_func = (rhs->flags & BSF_FUNCTION);
                        if (l_func == r_func) {
                            return bfd_asymbol_value(lhs) < bfd_asymbol_value(rhs);
                        } else {
                            return l_func;
                        }
                    });

                    /* Determine number of function symbols. */
                    m_num_func_symbol = 0;
                    for (size_t i = 0; i < m_num_symbol; ++i) {
                        if ((m_symbol[i]->flags & BSF_FUNCTION) == 0) {
                            m_num_func_symbol = i;
                            break;
                        }
                    }

                    for (int i = std::strlen(exe_path) - 1; i >= 0; --i) {
                        if (exe_path[i] == '/' || exe_path[i] == '\\') {
                            m_module_name = strdup(exe_path + i + 1);
                            break;
                        }
                    }

                    /* Get our module base/size. */
                    #if defined(ATMOSPHERE_OS_LINUX)
                    {
                        m_module_address = _r_debug.r_map->l_addr;

                        m_module_size = reinterpret_cast<uintptr_t>(std::addressof(__init_array_start)) - m_module_address;
                    }
                    #endif
                }

                ~BfdHelper() {
                    if (m_symbol != nullptr) {
                        std::free(m_symbol);
                    }
                    if (m_handle != nullptr) {
                       ::bfd_close(m_handle);
                    }
                }
            public:
                static BfdHelper &GetInstance() {
                    AMS_FUNCTION_LOCAL_STATIC(BfdHelper, s_bfd_helper_instance);
                    return s_bfd_helper_instance;
                }
            private:
                size_t GetSymbolSizeImpl(asymbol **symbol) const {
                    /* Do our best to guess. */
                    const auto vma = bfd_asymbol_value(*symbol);
                    if (symbol != m_symbol + m_num_func_symbol - 1) {
                        return bfd_asymbol_value(*(symbol + 1)) - vma;
                    } else {
                        const auto *sec = (*symbol)->section;
                        return (sec->vma + sec->size) - vma;
                    }
                }

                std::ptrdiff_t GetSymbolAddressDisplacement(uintptr_t address) const {
                    std::ptrdiff_t displacement = 0;

                    if (m_module_address <= address && address < m_module_address + m_module_size) {
                        displacement = m_module_address;
                    }

                    return displacement;
                }

                asymbol **GetBestSymbol(uintptr_t address) const {
                    /* Adjust the symbol address. */
                    address -= this->GetSymbolAddressDisplacement(address);

                    asymbol **best_symbol = std::lower_bound(m_symbol + 0, m_symbol + m_num_func_symbol, address, [](asymbol *lhs, uintptr_t rhs) {
                        return bfd_asymbol_value(lhs) < rhs;
                    });

                    if (best_symbol == m_symbol + m_num_func_symbol) {
                        return nullptr;
                    }

                    if (bfd_asymbol_value(*best_symbol) != address && best_symbol > m_symbol) {
                        --best_symbol;
                    }

                    const auto vma = bfd_asymbol_value(*best_symbol);
                    const auto end = vma + this->GetSymbolSizeImpl(best_symbol);

                    if (vma <= address && address < end) {
                        return best_symbol;
                    } else {
                        return nullptr;
                    }
                }
            public:
                uintptr_t GetSymbolName(char *dst, size_t dst_size, uintptr_t address) const {
                    /* Get the symbol. */
                    auto **symbol = this->GetBestSymbol(address);
                    if (symbol == nullptr) {
                        return 0;
                    }

                    /* Print the symbol. */
                    const char *name = bfd_asymbol_name(*symbol);

                    int cpp_name_status = 0;
                    if (char *demangled = abi::__cxa_demangle(name, nullptr, 0, std::addressof(cpp_name_status)); cpp_name_status == 0) {
                        AMS_ASSERT(demangled != nullptr);
                        util::TSNPrintf(dst, dst_size, "%s", demangled);
                        std::free(demangled);
                    } else {
                        util::TSNPrintf(dst, dst_size, "%s", name);
                    }

                    return bfd_asymbol_value(*symbol) + this->GetSymbolAddressDisplacement(address);
                }

                size_t GetSymbolSize(uintptr_t address) const {
                    /* Get the symbol. */
                    auto **symbol = this->GetBestSymbol(address);
                    if (symbol == nullptr) {
                        return 0;
                    }

                    return this->GetSymbolSizeImpl(symbol);
                }
            private:
                static void GetExecutablePath(char *dst, size_t dst_size) {
                    #if defined(ATMOSPHERE_OS_LINUX)
                    {
                        if (::readlink("/proc/self/exe", dst, dst_size) == -1) {
                            dst[0] = 0;
                            return;
                        }
                    }
                    #else
                    #error "Unknown OS for BfdHelper GetExecutablePath"
                    #endif
                }
        };

    }

    uintptr_t GetSymbolNameImpl(char *dst, size_t dst_size, uintptr_t address) {
        return BfdHelper::GetInstance().GetSymbolName(dst, dst_size, address);
    }

    size_t GetSymbolSizeImpl(uintptr_t address) {
        return BfdHelper::GetInstance().GetSymbolSize(address);
    }

}
