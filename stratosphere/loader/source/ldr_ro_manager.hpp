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
#include <switch.h>
#include <stratosphere.hpp>
#include <stratosphere/ldr.hpp>

namespace ams::ldr::ro {

    /* RO Manager API. */
    Result PinTitle(PinId *out, const ncm::TitleLocation &loc);
    Result UnpinTitle(PinId id);
    Result GetTitleLocation(ncm::TitleLocation *out, PinId id);
    Result RegisterProcess(PinId id, os::ProcessId process_id, ncm::TitleId title_id);
    Result RegisterModule(PinId id, const u8 *build_id, uintptr_t address, size_t size);
    Result GetProcessModuleInfo(u32 *out_count, ModuleInfo *out, size_t max_out_count, os::ProcessId process_id);

}
