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
#include <stratosphere.hpp>

namespace ams::dd {

    uintptr_t QueryIoMapping(uintptr_t phys_addr, size_t size) {
        u64 virtual_addr;
        const u64 aligned_addr = util::AlignDown(phys_addr, os::MemoryPageSize);
        const size_t offset = phys_addr - aligned_addr;
        const u64 aligned_size = size + offset;
        if (hos::GetVersion() >= hos::Version_10_0_0) {
            u64 region_size;
            R_TRY_CATCH(svcQueryIoMapping(&virtual_addr, &region_size, aligned_addr, aligned_size)) {
                /* Official software handles this by returning 0. */
                R_CATCH(svc::ResultNotFound) { return 0; }
            } R_END_TRY_CATCH_WITH_ABORT_UNLESS;
            AMS_ASSERT(region_size >= aligned_size);
        } else {
            R_TRY_CATCH(svcLegacyQueryIoMapping(&virtual_addr, aligned_addr, aligned_size)) {
                /* Official software handles this by returning 0. */
                R_CATCH(svc::ResultNotFound) { return 0; }
            } R_END_TRY_CATCH_WITH_ABORT_UNLESS;
        }

        return static_cast<uintptr_t>(virtual_addr + offset);
    }

    namespace {

        inline u32 ReadWriteRegisterImpl(uintptr_t phys_addr, u32 value, u32 mask) {
            u32 out_value;
            R_ABORT_UNLESS(svcReadWriteRegister(&out_value, phys_addr, mask, value));
            return out_value;
        }

    }

    u32 ReadRegister(uintptr_t phys_addr) {
        return ReadWriteRegisterImpl(phys_addr, 0, 0);
    }

    void WriteRegister(uintptr_t phys_addr, u32 value) {
        ReadWriteRegisterImpl(phys_addr, value, ~u32());
    }

    u32 ReadWriteRegister(uintptr_t phys_addr, u32 value, u32 mask) {
        return ReadWriteRegisterImpl(phys_addr, value, mask);
    }

}
