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
#include <mesosphere.hpp>

namespace ams::kern::svc {

    /* =============================    Common    ============================= */

    namespace {

        Result QueryProcessMemory(ams::svc::MemoryInfo *out_memory_info, ams::svc::PageInfo *out_page_info, ams::svc::Handle process_handle, uintptr_t address) {
            MESOSPHERE_LOG("%s: QueryProcessMemory(0x%08x, 0x%zx) was called\n", GetCurrentProcess().GetName(), process_handle, address);

            /* Get the process. */
            KScopedAutoObject process = GetCurrentProcess().GetHandleTable().GetObject<KProcess>(process_handle);
            R_UNLESS(process.IsNotNull(), svc::ResultInvalidHandle());

            /* Query the mapping's info. */
            KMemoryInfo info;
            R_TRY(process->GetPageTable().QueryInfo(std::addressof(info), out_page_info, address));

            /* Write output. */
            *out_memory_info = info.GetSvcMemoryInfo();
            return ResultSuccess();
        }

        Result QueryMemory(ams::svc::MemoryInfo *out_memory_info, ams::svc::PageInfo *out_page_info,  uintptr_t address) {
            /* Query memory is just QueryProcessMemory on the current process. */
            return QueryProcessMemory(out_memory_info, out_page_info, ams::svc::PseudoHandle::CurrentProcess, address);
        }

    }

    /* =============================    64 ABI    ============================= */

    Result QueryMemory64(KUserPointer<ams::svc::lp64::MemoryInfo *> out_memory_info, ams::svc::PageInfo *out_page_info, ams::svc::Address address) {
        /* Get an ams::svc::MemoryInfo for the region. */
        ams::svc::MemoryInfo info = {};
        R_TRY(QueryMemory(std::addressof(info), out_page_info, address));

        /* Try to copy to userspace. In the 64-bit case, ams::svc::lp64::MemoryInfo is the same as ams::svc::MemoryInfo. */
        static_assert(sizeof(ams::svc::MemoryInfo) == sizeof(ams::svc::lp64::MemoryInfo));
        R_TRY(out_memory_info.CopyFrom(std::addressof(info)));

        return ResultSuccess();
    }

    Result QueryProcessMemory64(KUserPointer<ams::svc::lp64::MemoryInfo *> out_memory_info, ams::svc::PageInfo *out_page_info, ams::svc::Handle process_handle, uint64_t address) {
        MESOSPHERE_PANIC("Stubbed SvcQueryProcessMemory64 was called.");
    }

    /* ============================= 64From32 ABI ============================= */

    Result QueryMemory64From32(KUserPointer<ams::svc::ilp32::MemoryInfo *> out_memory_info, ams::svc::PageInfo *out_page_info, ams::svc::Address address) {
        MESOSPHERE_PANIC("Stubbed SvcQueryMemory64From32 was called.");
    }

    Result QueryProcessMemory64From32(KUserPointer<ams::svc::ilp32::MemoryInfo *> out_memory_info, ams::svc::PageInfo *out_page_info, ams::svc::Handle process_handle, uint64_t address) {
        MESOSPHERE_PANIC("Stubbed SvcQueryProcessMemory64From32 was called.");
    }

}
