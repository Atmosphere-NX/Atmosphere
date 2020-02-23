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
#include "boot_pmc_wrapper.hpp"

namespace ams::boot {

    namespace {

        /* Convenience definitions. */
        constexpr u32 SmcFunctionId_AtmosphereReadWriteRegister = 0xF0000002;

        constexpr u32 PmcPhysStart = 0x7000E400;
        constexpr u32 PmcPhysEnd = 0x7000EFFF;

        /* Helpers. */
        bool IsValidPmcAddress(u32 phys_addr) {
            return (phys_addr & 3) == 0 && PmcPhysStart <= phys_addr && phys_addr <= PmcPhysEnd;
        }

        inline u32 ReadWriteRegisterImpl(uintptr_t phys_addr, u32 value, u32 mask) {
            u32 out_value;
            R_ABORT_UNLESS(spl::smc::ConvertResult(spl::smc::AtmosphereReadWriteRegister(phys_addr, mask, value, &out_value)));
            return out_value;
        }

    }

    u32 ReadPmcRegister(u32 phys_addr) {
        AMS_ABORT_UNLESS(IsValidPmcAddress(phys_addr));
        return ReadWriteRegisterImpl(phys_addr, 0, 0);
    }

    void WritePmcRegister(u32 phys_addr, u32 value, u32 mask) {
        AMS_ABORT_UNLESS(IsValidPmcAddress(phys_addr));
        ReadWriteRegisterImpl(phys_addr, value, mask);
    }

}
