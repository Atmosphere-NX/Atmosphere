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

namespace ams::os::impl {

    void SetMemoryAttributeImpl(uintptr_t address, size_t size, MemoryAttribute attr) {
        /* Determine svc arguments. */
        u32 svc_mask = svc::MemoryAttribute_Uncached;
        u32 svc_attr = 0;

        switch (attr) {
            case os::MemoryAttribute_Normal:   svc_attr = 0;                             break;
            case os::MemoryAttribute_Uncached: svc_attr = svc::MemoryAttribute_Uncached; break;
            AMS_UNREACHABLE_DEFAULT_CASE();
        }

        /* Loop, setting attribute. */
        auto cur_address = address;
        auto remaining   = size;
        while (remaining > 0) {
            /* Query the memory. */
            svc::MemoryInfo mem_info;
            svc::PageInfo page_info;
            R_ABORT_UNLESS(svc::QueryMemory(std::addressof(mem_info), std::addressof(page_info), cur_address));

            /* Determine the current size. */
            const size_t cur_size = std::min<size_t>(mem_info.base_address + mem_info.size - cur_address, remaining);

            /* Set the attribute, if necessary. */
            if (mem_info.attribute != svc_attr) {
                if (const auto res = svc::SetMemoryAttribute(address, size, svc_mask, svc_attr); R_FAILED(res)) {
                    /* NOTE: Nintendo logs here. */
                    R_ABORT_UNLESS(res);
                }
            }

            /* Advance. */
            cur_address += cur_size;
            remaining   -= cur_size;
        }
    }

}
