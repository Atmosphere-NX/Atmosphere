/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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

namespace sts::boot {

    namespace {

        /* Convenience definitions. */
        constexpr u32 SmcFunctionId_AtmosphereReadWriteRegister = 0xF0000002;

        constexpr u32 PmcPhysStart = 0x7000E400;
        constexpr u32 PmcPhysEnd = 0x7000EFFF;

        /* Helpers. */
        bool IsValidPmcAddress(u32 phys_addr) {
            return (phys_addr & 3) == 0 && PmcPhysStart <= phys_addr && phys_addr <= PmcPhysEnd;
        }

        inline u32 SmcAtmosphereReadWriteRegister(u32 phys_addr, u32 value, u32 mask) {
            SecmonArgs args;

            args.X[0] = SmcFunctionId_AtmosphereReadWriteRegister;
            args.X[1] = phys_addr;
            args.X[2] = mask;
            args.X[3] = value;
            R_ASSERT(svcCallSecureMonitor(&args));
            if (args.X[0] != 0) {
                std::abort();
            }

            return static_cast<u32>(args.X[1]);
        }

    }

    u32 ReadPmcRegister(u32 phys_addr) {
        if (!IsValidPmcAddress(phys_addr)) {
            std::abort();
        }

        return SmcAtmosphereReadWriteRegister(phys_addr, 0, 0);
    }

    void WritePmcRegister(u32 phys_addr, u32 value, u32 mask) {
        if (!IsValidPmcAddress(phys_addr)) {
            std::abort();
        }

        SmcAtmosphereReadWriteRegister(phys_addr, value, mask);
    }

}
