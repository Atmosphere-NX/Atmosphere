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
#include <exosphere.hpp>
#include "fatal_display.hpp"
#include "fatal_print.hpp"

namespace ams::secmon::fatal {

    namespace {

        #include "fatal_font.inc"

        constexpr inline const u32 TextColor = 0xFFA0A0A0;

        constexpr inline const size_t ConsoleWidth  = FrameBufferWidth  / FontWidth;
        constexpr inline const size_t ConsoleHeight = FrameBufferHeight / FontHeight;

        constinit u32 *g_frame_buffer = nullptr;
        constinit size_t g_col = 1;
        constinit size_t g_row = 0;

        void SetPixel(size_t x, size_t y, u32 color) {
            g_frame_buffer[(FrameBufferWidth - x) * FrameBufferHeight + y] = color;
        }

        void PutCarriageReturn() {
            g_col = 1;
        }

        void PutNewLine() {
            g_col = 1;
            ++g_row;

            /* TODO: Support scrolling? */
        }

        void PutCharImpl(const char c) {
            /* Get the character data for the font. */
            const u8 * cdata = FontData + c * (FontHeight * util::DivideUp(FontWidth, BITSIZEOF(u8)));

            /* Determine where to start drawing. */
            const size_t x = g_col * FontWidth;
            const size_t y = g_row * FontHeight;

            for (size_t cur_y = 0; cur_y < FontHeight; ++cur_y) {
                size_t cur_x = 0;
                int wbits = FontWidth;
                while (wbits > 0) {
                    const auto bits = *(cdata++);

                    SetPixel(x + cur_x + 0, y + cur_y, FontDrawTable[(bits >> 4) & 0xF][0] & TextColor);
                    SetPixel(x + cur_x + 1, y + cur_y, FontDrawTable[(bits >> 4) & 0xF][1] & TextColor);
                    SetPixel(x + cur_x + 2, y + cur_y, FontDrawTable[(bits >> 4) & 0xF][2] & TextColor);
                    SetPixel(x + cur_x + 3, y + cur_y, FontDrawTable[(bits >> 4) & 0xF][3] & TextColor);
                    SetPixel(x + cur_x + 4, y + cur_y, FontDrawTable[(bits >> 0) & 0xF][0] & TextColor);
                    SetPixel(x + cur_x + 5, y + cur_y, FontDrawTable[(bits >> 0) & 0xF][1] & TextColor);
                    SetPixel(x + cur_x + 6, y + cur_y, FontDrawTable[(bits >> 0) & 0xF][2] & TextColor);
                    SetPixel(x + cur_x + 7, y + cur_y, FontDrawTable[(bits >> 0) & 0xF][3] & TextColor);

                    cur_x += BITSIZEOF(u8);
                    wbits -= BITSIZEOF(u8);
                }
            }
        }

        void PutChar(const char c) {
            switch (c) {
                case '\r':
                    PutCarriageReturn();
                    break;
                case '\n':
                    PutNewLine();
                    break;
                default:
                    PutCharImpl(c);
                    if ((++g_col) >= ConsoleWidth) {
                        PutNewLine();
                    }
            }
        }

    }

    void InitializeConsole(u32 *frame_buffer) {
        /* Setup the console variables. */
        g_frame_buffer = frame_buffer;
        g_col          = 1;
        g_row          = 0;

        /* Clear the console. */
        std::memset(g_frame_buffer, 0, FrameBufferSize);
    }

    void Print(const char *fmt, ...) {
        /* Generate the string. */
        char log_str[1_KB];
        {
            std::va_list vl;
            va_start(vl, fmt);
            util::TVSNPrintf(log_str, sizeof(log_str), fmt, vl);
            va_end(vl);
        }

        /* Print each character. */
        const size_t len = std::strlen(log_str);
        for (size_t i = 0; i < len; ++i) {
            PutChar(log_str[i]);
        }

        /* Flush the console. */
        hw::FlushDataCache(g_frame_buffer, FrameBufferSize);
    }

}
