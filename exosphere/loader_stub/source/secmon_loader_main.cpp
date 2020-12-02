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
#include <exosphere.hpp>
#include "secmon_loader_uncompress.hpp"
#include "program_lz4.h"
#include "boot_code_lz4.h"

namespace ams::secmon::loader {

    NORETURN void UncompressAndExecute() {
        /* Uncompress the program image. */
        Uncompress(secmon::MemoryRegionPhysicalTzramFullProgramImage.GetPointer(), secmon::MemoryRegionPhysicalTzramFullProgramImage.GetSize(), program_lz4, program_lz4_size);

        /* Copy the boot image to the end of IRAM */
        u8 *relocated_boot_code = secmon::MemoryRegionPhysicalIramBootCodeImage.GetEndPointer<u8>() - boot_code_lz4_size;
        std::memcpy(relocated_boot_code, boot_code_lz4, boot_code_lz4_size);

        /* Uncompress the boot image. */
        Uncompress(secmon::MemoryRegionPhysicalIramBootCodeImage.GetPointer(), secmon::MemoryRegionPhysicalIramBootCodeImage.GetSize(), relocated_boot_code, boot_code_lz4_size);

        /* Jump to the boot image. */
        reinterpret_cast<void (*)()>(secmon::MemoryRegionPhysicalIramBootCodeImage.GetAddress())();

        /* We will never reach this point. */
        __builtin_unreachable();
    }

}