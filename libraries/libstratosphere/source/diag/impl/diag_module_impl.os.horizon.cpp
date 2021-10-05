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

namespace ams::diag::impl {

    namespace {

        constexpr inline uintptr_t ModulePathLengthOffset = 4;
        constexpr inline uintptr_t ModulePathOffset       = 8;

    }

    uintptr_t GetModuleInfoForHorizon(const char **out_path, size_t *out_path_length, size_t *out_module_size, uintptr_t address) {
        /* Check for null address. */
        if (address == 0) {
            return 0;
        }

        /* Get module info. */
        ro::impl::ExceptionInfo exception_info;
        if (!ro::impl::GetExceptionInfo(std::addressof(exception_info), address)) {
            return 0;
        }

        /* Locate the path in the first non-read-execute segment. */
        svc::MemoryInfo mem_info;
        svc::PageInfo page_info;
        auto cur_address = exception_info.module_address;
        while (cur_address < exception_info.module_address + exception_info.module_size) {
            if (R_FAILED(svc::QueryMemory(std::addressof(mem_info), std::addressof(page_info), cur_address))) {
                return 0;
            }
            if (mem_info.permission != svc::MemoryPermission_ReadExecute) {
                break;
            }
            cur_address += mem_info.size;
        }

        /* Set output info. */
        *out_path        = reinterpret_cast<const char *>(cur_address + ModulePathOffset);
        *out_path_length = *reinterpret_cast<const u32 *>(cur_address + ModulePathLengthOffset);
        *out_module_size = exception_info.module_size;

        return exception_info.module_address;
    }

}
