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

namespace sts::ldr::args {

    constexpr size_t ArgumentSizeMax = 0x8000;

    struct ArgumentInfo {
        ncm::TitleId title_id;
        size_t args_size;
        u8 args[ArgumentSizeMax];
    };

    /* API. */
    const ArgumentInfo *Get(ncm::TitleId title_id);
    Result Set(ncm::TitleId title_id, const void *args, size_t args_size);
    Result Clear();

}
