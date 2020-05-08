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
#include <stratosphere.hpp>
#include "ro_patcher.hpp"

namespace ams::ro::impl {

    namespace {

        constexpr const char *NroPatchesDirectory = "nro_patches";

        /* NRO patches want to prevent modification of header, */
        /* but don't want to adjust offset relative to mapped location. */
        constexpr size_t NroPatchesProtectedSize = sizeof(NroHeader);
        constexpr size_t NroPatchesProtectedOffset = 0;

    }

    /* Apply IPS patches. */
    void LocateAndApplyIpsPatchesToModule(const ModuleId *module_id, u8 *mapped_nro, size_t mapped_size) {
        ams::patcher::LocateAndApplyIpsPatchesToModule("sdmc", NroPatchesDirectory, NroPatchesProtectedSize, NroPatchesProtectedOffset, module_id, mapped_nro, mapped_size);
    }

}