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
#include "ldr_patcher.hpp"

namespace ams::ldr {

    namespace {

        constexpr const char *NsoPatchesDirectory = "exefs_patches";

        /* Exefs patches want to prevent modification of header, */
        /* and also want to adjust offset relative to mapped location. */
        constexpr size_t NsoPatchesProtectedSize   = sizeof(NsoHeader);
        constexpr size_t NsoPatchesProtectedOffset = sizeof(NsoHeader);

    }

    /* Apply IPS patches. */
    void LocateAndApplyIpsPatchesToModule(const u8 *build_id, uintptr_t mapped_nso, size_t mapped_size) {
        ro::ModuleId module_id;
        std::memcpy(&module_id.build_id, build_id, sizeof(module_id.build_id));
        ams::patcher::LocateAndApplyIpsPatchesToModule(NsoPatchesDirectory, NsoPatchesProtectedSize, NsoPatchesProtectedOffset, &module_id, reinterpret_cast<u8 *>(mapped_nso), mapped_size);
    }

}