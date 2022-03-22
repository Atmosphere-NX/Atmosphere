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
#include "fusee_loader_uncompress.hpp"
#include "program_lz4.h"

namespace ams::nxboot::loader {

    namespace {

        constexpr uintptr_t ProgramImageBase = 0x40001000;
        constexpr uintptr_t ProgramImageEnd  = 0x4003D000;
        constexpr size_t ProgramImageSizeMax = ProgramImageEnd - ProgramImageBase;

        void CopyBackwards(void *dst, const void *src, size_t size) {
                  u8 *dst_8 = static_cast<u8 *>(dst) + size;
            const u8 *src_8 = static_cast<const u8 *>(src) + size;

            for (size_t i = 0; i < size; ++i) {
                *(--dst_8) = *(--src_8);
            }
        }

    }

    NORETURN void UncompressAndExecute(const void *program, size_t program_size) {
        /* Relocate the compressed binary to a place where we can safely decompress it. */
        void *relocated_program = reinterpret_cast<void *>(ProgramImageEnd - program_size);
        if (relocated_program != program) {
            CopyBackwards(relocated_program, program, program_size);
        }

        /* Uncompress the program image. */
        Uncompress(reinterpret_cast<void *>(ProgramImageBase), ProgramImageSizeMax, relocated_program, program_size);

        /* Jump to the boot image. */
        reinterpret_cast<void (*)()>(ProgramImageBase)();

        /* We will never reach this point. */
        __builtin_unreachable();
    }

}