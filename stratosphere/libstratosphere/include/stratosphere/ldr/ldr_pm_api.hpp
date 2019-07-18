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

#include "ldr_types.hpp"

namespace sts::ldr::pm {

    /* Process Manager API. */
    Result CreateProcess(Handle *out, PinId pin_id, u32 flags, Handle reslimit);
    Result GetProgramInfo(ProgramInfo *out, const ncm::TitleLocation &loc);
    Result PinTitle(PinId *out, const ncm::TitleLocation &loc);
    Result UnpinTitle(PinId pin_id);
    Result HasLaunchedTitle(bool *out, ncm::TitleId title_id);

}
