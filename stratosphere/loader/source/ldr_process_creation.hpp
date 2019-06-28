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

#include "ldr_arguments.hpp"

namespace sts::ldr {

    /* Process Creation API. */
    Result CreateProcess(Handle *out, PinId pin_id, const ncm::TitleLocation &loc, const char *path, u32 flags, Handle reslimit_h);
    Result GetProgramInfo(ProgramInfo *out, const ncm::TitleLocation &loc);

}
