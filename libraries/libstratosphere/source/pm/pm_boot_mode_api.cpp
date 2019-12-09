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
#include <stratosphere.hpp>

namespace ams::pm::bm {

    /* Boot Mode API. */
    /* Both functions should be weakly linked, so that they can be overridden by ams::boot2 as needed. */
    BootMode WEAK GetBootMode() {
        PmBootMode boot_mode = PmBootMode_Normal;
        R_ASSERT(pmbmGetBootMode(&boot_mode));
        return static_cast<BootMode>(boot_mode);
    }

    void WEAK SetMaintenanceBoot() {
        R_ASSERT(pmbmSetMaintenanceBoot());
    }

}
