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

namespace ams::ro::impl {

    namespace {

        bool SearchSegmentHead(uintptr_t *out, uintptr_t target, svc::MemoryState state) {
            svc::MemoryInfo mem_info;
            svc::PageInfo page_info;
            bool success = false;

            for (uintptr_t cur = target; cur <= target; cur = mem_info.base_address - 1) {
                R_ABORT_UNLESS(svc::QueryMemory(std::addressof(mem_info), std::addressof(page_info), cur));

                if (mem_info.state != state || mem_info.permission != svc::MemoryPermission_ReadExecute) {
                    break;
                }

                *out = mem_info.base_address;
                success = true;
            }

            return success;
        }

        bool SearchSegmentTail(uintptr_t *out, uintptr_t target, svc::MemoryState state) {
            svc::MemoryInfo mem_info;
            svc::PageInfo page_info;
            bool success = false;

            for (uintptr_t cur = target; cur >= target; cur = mem_info.base_address + mem_info.size) {
                R_ABORT_UNLESS(svc::QueryMemory(std::addressof(mem_info), std::addressof(page_info), cur));

                if (mem_info.state != state) {
                    break;
                }

                *out = mem_info.base_address + mem_info.size - 1;
                success = true;
            }

            return success;
        }

        bool QueryModule(uintptr_t *out_address, size_t *out_size, uintptr_t pc) {
            /* Query the program counter. */
            svc::MemoryInfo mem_info;
            svc::PageInfo page_info;
            R_ABORT_UNLESS(svc::QueryMemory(std::addressof(mem_info), std::addressof(page_info), pc));

            /* Check memory info. */
            if (mem_info.permission != svc::MemoryPermission_ReadExecute) {
                return false;
            }

            if (mem_info.state != svc::MemoryState_Code && mem_info.state != svc::MemoryState_AliasCode) {
                return false;
            }

            /* Find head/tail. */
            uintptr_t head = 0, tail = 0;
            AMS_ABORT_UNLESS(SearchSegmentHead(std::addressof(head), pc, mem_info.state));
            AMS_ABORT_UNLESS(SearchSegmentTail(std::addressof(tail), pc, mem_info.state));
            AMS_ABORT_UNLESS(SearchSegmentTail(std::addressof(tail), tail + 1, mem_info.state == svc::MemoryState_Code ? svc::MemoryState_CodeData : svc::MemoryState_AliasCodeData));

            /* Set output. */
            *out_address = head;
            *out_size    = tail + 1 - head;

            return true;
        }

    }

    bool GetExceptionInfo(ExceptionInfo *out, uintptr_t pc) {
        /* Check pre-conditions. */
        AMS_ASSERT(out != nullptr);

        /* Find the module. */
        if (!QueryModule(std::addressof(out->module_address), std::addressof(out->module_size), pc)) {
            return false;
        }

        /* Validate the module. */
        rocrt::ModuleHeaderLocation *loc = reinterpret_cast<rocrt::ModuleHeaderLocation *>(out->module_address);
        rocrt::ModuleHeader *header      = rocrt::GetModuleHeader(loc);
        AMS_ABORT_UNLESS(header->signature == rocrt::ModuleHeaderVersion);

        /* Set the exception info. */
        out->info_offset = loc->header_offset + header->exception_info_start_offset;
        out->info_size   = header->exception_info_end_offset - header->exception_info_start_offset;

        return true;
    }

}
