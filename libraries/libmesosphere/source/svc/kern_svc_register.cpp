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

        Result ReadWriteRegister(uint32_t *out, ams::svc::PhysicalAddress address, uint32_t mask, uint32_t value) {
            /* Clear the output unconditionally. */
            *out = 0;

            /* Read/write the register. */
            return KSystemControl::ReadWriteRegister(out, address, mask, value);
        }

    }

    /* =============================    64 ABI    ============================= */

    Result ReadWriteRegister64(uint32_t *out_value, ams::svc::PhysicalAddress address, uint32_t mask, uint32_t value) {
        return ReadWriteRegister(out_value, address, mask, value);
    }

    /* ============================= 64From32 ABI ============================= */

    Result ReadWriteRegister64From32(uint32_t *out_value, ams::svc::PhysicalAddress address, uint32_t mask, uint32_t value) {
        return ReadWriteRegister(out_value, address, mask, value);
    }

}
