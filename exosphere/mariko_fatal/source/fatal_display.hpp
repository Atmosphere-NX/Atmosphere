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
#pragma once
#include <exosphere.hpp>

namespace ams::secmon::fatal {

    constexpr inline size_t FrameBufferHeight = 768;
    constexpr inline size_t FrameBufferWidth  = 1280;
    constexpr inline size_t FrameBufferSize   = FrameBufferHeight * FrameBufferWidth * sizeof(u32);

    void InitializeDisplay();
    void ShowDisplay(const ams::impl::FatalErrorContext *f_ctx, const Result save_result);

}
