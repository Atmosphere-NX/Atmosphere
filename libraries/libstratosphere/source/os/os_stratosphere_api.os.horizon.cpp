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
#include "impl/os_resource_manager.hpp"

extern "C" { extern u8 __argdata__[]; }

namespace ams::os {

    namespace {

        class MemoryArranger {
            private:
                uintptr_t m_address;
            public:
                constexpr MemoryArranger(uintptr_t address) : m_address(address) { /* ... */ }

                template<typename T>
                T *Arrange() {
                    this->Align(alignof(T));
                    return static_cast<T *>(this->Arrange(sizeof(T)));
                }

                void *Arrange(size_t size) {
                    const auto address = m_address;
                    m_address += size;
                    return reinterpret_cast<void *>(address);
                }

                void Align(size_t align) {
                    m_address = util::AlignUp(m_address, align);
                }

                char *ArrangeCharArray(size_t size) {
                    return reinterpret_cast<char *>(Arrange(size));
                }
        };

        bool HasArguments(uintptr_t args_region) {
            /* Check that the arguments region is read-write. */
            svc::MemoryInfo mi;
            svc::PageInfo pi;

            if (R_FAILED(svc::QueryMemory(std::addressof(mi), std::addressof(pi), args_region))) {
                return false;
            }

            return mi.permission == svc::MemoryPermission_ReadWrite;
        }

        const char *SkipSpace(const char *p, const char *end) {
            while (p < end && std::isspace(*p)) { ++p; }
            return p;
        }

        const char *GetTokenEnd(const char *p, const char *end) {
            while (p < end && !std::isspace(*p)) { ++p; }
            return p;
        }

        const char *GetQuotedTokenEnd(const char *p, const char *end) {
            while (p < end && *p != '"') { ++p; }
            return p;
        }

        int MakeArgv(char **out_argv_buf, char *arg_buf, const char *cmd_line, size_t cmd_line_size, int arg_max) {
            /* Prepare to parse arguments. */
                  auto idx = 0;
                  auto src = cmd_line;
                  auto dst = arg_buf;
            const auto end = src + cmd_line_size;

            /* Parse all tokens. */
            while (true) {
                /* Advance past any spaces. */
                src = SkipSpace(src, end);
                if (src >= end) {
                    break;
                }

                /* Check that we don't have too many arguments. */
                if (idx >= arg_max) {
                    break;
                }

                /* Find the start/end of the current argument token. */
                const char *arg_end;
                const char *src_next;
                if (*src == '"') {
                    ++src;
                    arg_end  = GetQuotedTokenEnd(src, end);
                    src_next = arg_end + 1;
                } else {
                    arg_end  = GetTokenEnd(src, end);
                    src_next = arg_end;
                }

                /* Determine token size. */
                const auto arg_size = arg_end - src;

                /* Set the argv pointer. */
                out_argv_buf[idx++] = dst;

                /* Copy the argument. */
                std::memcpy(dst, src, arg_size);
                dst += arg_size;

                /* Null-terminate the argument token. */
                *(dst++) = '\x00';

                /* Advance to next token. */
                src = src_next;
            }

            /* Null terminate the final token. */
            *(dst++) = '\x00';

            /* Null terminate argv. */
            out_argv_buf[idx] = nullptr;

            return idx;
        }


    }

    void SetHostArgc(int argc);
    void SetHostArgv(char **argv);

    void InitializeForStratosphereInternal() {
        /* Initialize the global os resource manager. */
        os::impl::ResourceManagerHolder::InitializeResourceManagerInstance();

        /* Setup host argc/argv as needed. */
        const uintptr_t args_region = reinterpret_cast<uintptr_t>(__argdata__);
        if (HasArguments(args_region)) {
            /* Create arguments memory arranger. */
            MemoryArranger arranger(args_region);

            /* Arrange. */
            const auto &header   = *arranger.Arrange<ldr::ProgramArguments>();
            const char *cmd_line =  arranger.ArrangeCharArray(header.arguments_size);
            char *arg_buf        =  arranger.ArrangeCharArray(header.arguments_size + 2);
            char **argv_buf      =  arranger.Arrange<char *>();

            /* Determine extents. */
            const auto arg_buf_size = reinterpret_cast<uintptr_t>(argv_buf) - args_region;
            const auto arg_max      = (header.allocated_size - arg_buf_size) / sizeof(char *);

            /* Make argv. */
            const auto arg_count = MakeArgv(argv_buf, arg_buf, cmd_line, header.arguments_size, arg_max);

            /* Set host argc/argv. */
            os::SetHostArgc(arg_count);
            os::SetHostArgv(argv_buf);
        }
    }

}
