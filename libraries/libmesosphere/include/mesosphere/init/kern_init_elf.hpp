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
#pragma once
#include <vapours.hpp>

#ifdef ATMOSPHERE_ARCH_ARM64
    #include "kern_init_elf64.hpp"
#else
    #error "Unknown Architecture"
#endif

namespace ams::kern::init::Elf {

    #ifdef ATMOSPHERE_ARCH_ARM64
        using namespace ams::kern::init::Elf::Elf64;

        enum RelocationType {
            R_ARCHITECTURE_RELATIVE = 0x403, /* Real name R_AARCH64_RELATIVE */
        };
    #else
        #error "Unknown Architecture"
    #endif

    /* API to apply relocations or call init array. */
    void ApplyRelocations(uintptr_t base_address, const Dyn *dynamic);
    void CallInitArrayFuncs(uintptr_t init_array_start, uintptr_t init_array_end);

}