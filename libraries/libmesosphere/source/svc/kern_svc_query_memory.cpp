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

        template<typename T>
        Result QueryProcessMemory(KUserPointer<T *> out_memory_info, ams::svc::PageInfo *out_page_info, ams::svc::Handle process_handle, uint64_t address) {
            /* Get an ams::svc::MemoryInfo for the region. */
            ams::svc::MemoryInfo info = {};
            R_TRY(QueryProcessMemory(std::addressof(info), out_page_info, process_handle, address));

            /* Copy the info to userspace. */
            if constexpr (std::same_as<T, ams::svc::MemoryInfo>) {
                R_TRY(out_memory_info.CopyFrom(std::addressof(info)));
            } else {
                /* Convert the info. */
                T converted_info = {};
                static_assert(std::same_as<decltype(T{}.addr), decltype(ams::svc::MemoryInfo{}.addr)>);
                static_assert(std::same_as<decltype(T{}.size), decltype(ams::svc::MemoryInfo{}.size)>);

                converted_info.addr             = info.addr;
                converted_info.size             = info.size;
                converted_info.state            = info.state;
                converted_info.attr             = info.attr;
                converted_info.perm             = info.perm;
                converted_info.ipc_refcount     = info.ipc_refcount;
                converted_info.device_refcount  = info.device_refcount;

                /* Copy it. */
                R_TRY(out_memory_info.CopyFrom(std::addressof(converted_info)));
            }

            return ResultSuccess();
        }


        template<typename T>
        Result QueryMemory(KUserPointer<T *> out_memory_info, ams::svc::PageInfo *out_page_info, uintptr_t address) {
            /* Query memory is just QueryProcessMemory on the current process. */
            return QueryProcessMemory(out_memory_info, out_page_info, ams::svc::PseudoHandle::CurrentProcess, address);
        }

    }

    /* =============================    64 ABI    ============================= */

    Result QueryMemory64(KUserPointer<ams::svc::lp64::MemoryInfo *> out_memory_info, ams::svc::PageInfo *out_page_info, ams::svc::Address address) {
        return QueryMemory(out_memory_info, out_page_info, address);
    }

    Result QueryProcessMemory64(KUserPointer<ams::svc::lp64::MemoryInfo *> out_memory_info, ams::svc::PageInfo *out_page_info, ams::svc::Handle process_handle, uint64_t address) {
        return QueryProcessMemory(out_memory_info, out_page_info, process_handle, address);
    }

    /* ============================= 64From32 ABI ============================= */

    Result QueryMemory64From32(KUserPointer<ams::svc::ilp32::MemoryInfo *> out_memory_info, ams::svc::PageInfo *out_page_info, ams::svc::Address address) {
        return QueryMemory(out_memory_info, out_page_info, address);
    }

    Result QueryProcessMemory64From32(KUserPointer<ams::svc::ilp32::MemoryInfo *> out_memory_info, ams::svc::PageInfo *out_page_info, ams::svc::Handle process_handle, uint64_t address) {
        return QueryProcessMemory(out_memory_info, out_page_info, process_handle, address);
    }

}
