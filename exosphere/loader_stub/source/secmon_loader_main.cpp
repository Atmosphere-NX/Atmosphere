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
#include <exosphere.hpp>
#include "secmon_loader_uncompress.hpp"

namespace ams::secmon::loader {

    namespace {

        constexpr const u8 SecmonProgramLz4[] = {
            #embed <program.lz4>
        };

        constexpr const u8 SecmonBootCodeLz4[] = {
            #embed <boot_code.lz4>
        };

    }

    NORETURN void UncompressAndExecute() {
        /* Uncompress the program image. */
        Uncompress(secmon::MemoryRegionPhysicalTzramFullProgramImage.GetPointer(), secmon::MemoryRegionPhysicalTzramFullProgramImage.GetSize(), SecmonProgramLz4, sizeof(SecmonProgramLz4));

        /* Copy the boot image to the end of IRAM */
        u8 *relocated_boot_code = secmon::MemoryRegionPhysicalIramBootCodeImage.GetEndPointer<u8>() - sizeof(SecmonBootCodeLz4);
        std::memcpy(relocated_boot_code, SecmonBootCodeLz4, sizeof(SecmonBootCodeLz4));

        /* Uncompress the boot image. */
        Uncompress(secmon::MemoryRegionPhysicalIramBootCodeImage.GetPointer(), secmon::MemoryRegionPhysicalIramBootCodeImage.GetSize(), relocated_boot_code, sizeof(SecmonBootCodeLz4));

        /* Jump to the boot image. */
        reinterpret_cast<void (*)()>(secmon::MemoryRegionPhysicalIramBootCodeImage.GetAddress())();

        /* We will never reach this point. */
        __builtin_unreachable();
    }

}