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

#pragma once

#include <stratosphere/ro/ro_types.hpp>

namespace ams::patcher {

    /* Helper for applying to code binaries. */
    void LocateAndApplyIpsPatchesToModule(const char *mount_name, const char *patch_dir, size_t protected_size, size_t offset, const ro::ModuleId *module_id, u8 *mapped_module, size_t mapped_size);

}
