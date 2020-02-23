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



    }

    /* =============================    64 ABI    ============================= */

    Result QueryPhysicalAddress64(ams::svc::lp64::PhysicalMemoryInfo *out_info, ams::svc::Address address) {
        MESOSPHERE_PANIC("Stubbed SvcQueryPhysicalAddress64 was called.");
    }

    Result QueryIoMapping64(ams::svc::Address *out_address, ams::svc::PhysicalAddress physical_address, ams::svc::Size size) {
        MESOSPHERE_PANIC("Stubbed SvcQueryIoMapping64 was called.");
    }

    /* ============================= 64From32 ABI ============================= */

    Result QueryPhysicalAddress64From32(ams::svc::ilp32::PhysicalMemoryInfo *out_info, ams::svc::Address address) {
        MESOSPHERE_PANIC("Stubbed SvcQueryPhysicalAddress64From32 was called.");
    }

    Result QueryIoMapping64From32(ams::svc::Address *out_address, ams::svc::PhysicalAddress physical_address, ams::svc::Size size) {
        MESOSPHERE_PANIC("Stubbed SvcQueryIoMapping64From32 was called.");
    }

}
