/*
 * Copyright (c) 2019-2020 Atmosphère-NX
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
#include <stratosphere.hpp>

namespace ams::capsrv::server {

    struct DecoderWorkMemory {
        alignas(os::MemoryPageSize) u8 jpeg_decoder_memory[SoftwareJpegDecoderWorkMemorySize];
    };
    static_assert(sizeof(DecoderWorkMemory)  == SoftwareJpegDecoderWorkMemorySize);
    static_assert(alignof(DecoderWorkMemory) == os::MemoryPageSize);
    static_assert(util::is_pod<DecoderWorkMemory>::value);

}
