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
#include <stratosphere.hpp>

namespace ams::fatal::srv::font {

    Result InitializeSharedFont();
    void ConfigureFontFramebuffer(u16 *fb, u32 (*unswizzle_func)(u32, u32));
    void SetHeapMemory(void *memory, size_t memory_size);

    void SetFontColor(u16 color);
    void SetPosition(u32 x, u32 y);
    u32 GetX();
    u32 GetY();
    void SetFontSize(float fsz);
    void AddSpacingLines(float num_lines);
    void PrintLine(const char *str);
    void PrintFormatLine(const char *format, ...);
    void Print(const char *str);
    void PrintFormat(const char *format, ...);
    void PrintMonospaceU64(u64 x);
    void PrintMonospaceU32(u32 x);
    void PrintMonospaceBlank(u32 width);

}
