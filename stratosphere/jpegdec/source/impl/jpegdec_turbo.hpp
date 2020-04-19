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
#include <stratosphere.hpp>

namespace ams::jpegdec::impl {

    struct DecodeInput {
        const u8 *jpeg;
        size_t jpeg_size;
        u32 width;
        u32 height;
        bool fancy_upsampling;
        bool block_smoothing;
    };

    struct DecodeOutput {
        u32 *width;
        u32 *height;
        u8 *bmp;
        size_t bmp_size;
    };

    struct Dimensions {
        u32 width, height;
    };

    Result DecodeJpeg(DecodeOutput &out, const DecodeInput &in, u8 *work, size_t work_size);

}