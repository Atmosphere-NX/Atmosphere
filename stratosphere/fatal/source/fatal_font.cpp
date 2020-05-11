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
#include <stratosphere.hpp>
#include "fatal_config.hpp"
#include "fatal_font.hpp"

#define STBTT_STATIC
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"
#undef  STBTT_STATIC
#undef  STB_TRUETYPE_IMPLEMENTATION

/* Define color conversion macros. */
#define RGB888_TO_RGB565(r, g, b) ((((r >> 3) << 11) & 0xF800) | (((g >> 2) << 5) & 0x7E0) | ((b >> 3) & 0x1F))
#define RGB565_GET_R8(c) ((((c >> 11) & 0x1F) << 3) | ((c >> 13) & 7))
#define RGB565_GET_G8(c) ((((c >> 5) & 0x3F) << 2) | ((c >> 9) & 3))
#define RGB565_GET_B8(c) ((((c >> 0) & 0x1F) << 3) | ((c >> 2) & 7))

namespace ams::fatal::srv::font {

    namespace {

        /* Font state globals. */
        u16 *g_frame_buffer = nullptr;
        u32 (*g_unswizzle_func)(u32, u32) = nullptr;
        u16 g_font_color = 0xFFFF; /* White. */
        float g_font_line_pixels = 16.0f;
        float g_font_size = 16.0f;
        u32 g_line_x = 0, g_cur_x = 0, g_cur_y = 0;

        u32 g_mono_adv = 0;

        PlFontData g_font;

        stbtt_fontinfo g_stb_font;

        /* Helpers. */
        u16 Blend(u16 color, u16 bg, u8 alpha) {
            const u32 c_r = RGB565_GET_R8(color);
            const u32 c_g = RGB565_GET_G8(color);
            const u32 c_b = RGB565_GET_B8(color);
            const u32 b_r = RGB565_GET_R8(bg);
            const u32 b_g = RGB565_GET_G8(bg);
            const u32 b_b = RGB565_GET_B8(bg);

            const u32 r = ((alpha * c_r) + ((0xFF - alpha) * b_r)) / 0xFF;
            const u32 g = ((alpha * c_g) + ((0xFF - alpha) * b_g)) / 0xFF;
            const u32 b = ((alpha * c_b) + ((0xFF - alpha) * b_b)) / 0xFF;

            return RGB888_TO_RGB565(r, g, b);
        }

        void DrawCodePoint(u32 codepoint, u32 x, u32 y) {
            int width = 0, height = 0;
            u8* imageptr = stbtt_GetCodepointBitmap(&g_stb_font, g_font_size, g_font_size, codepoint, &width, &height, 0, 0);
            ON_SCOPE_EXIT { std::free(imageptr); };

            for (int tmpy = 0; tmpy < height; tmpy++) {
                for (int tmpx = 0; tmpx < width; tmpx++) {
                    /* Implement very simple blending, as the bitmap value is an alpha value. */
                    u16 *ptr = &g_frame_buffer[g_unswizzle_func(x + tmpx, y + tmpy)];
                    *ptr = Blend(g_font_color, *ptr, imageptr[width * tmpy + tmpx]);
                }
            }
        }

        void DrawString(const char *str, bool add_line, bool mono = false) {
            const size_t len = strlen(str);

            u32 cur_x = g_cur_x, cur_y = g_cur_y;

            u32 prev_char = 0;
            for (u32 i = 0; i < len; ) {
                u32 cur_char;
                ssize_t unit_count = decode_utf8(&cur_char, reinterpret_cast<const u8 *>(&str[i]));
                if (unit_count <= 0) break;

                if (!g_mono_adv && i > 0) {
                    cur_x += g_font_size * stbtt_GetCodepointKernAdvance(&g_stb_font, prev_char, cur_char);
                }

                i += unit_count;

                if (cur_char == '\n') {
                    cur_x = g_line_x;
                    cur_y += g_font_line_pixels;
                    continue;
                }

                int adv_width, left_side_bearing;
                stbtt_GetCodepointHMetrics(&g_stb_font, cur_char, &adv_width, &left_side_bearing);
                const u32 cur_width = static_cast<u32>(adv_width) * g_font_size;

                int x0, y0, x1, y1;
                stbtt_GetCodepointBitmapBoxSubpixel(&g_stb_font, cur_char, g_font_size, g_font_size, 0, 0, &x0, &y0, &x1, &y1);

                DrawCodePoint(cur_char, cur_x + x0 + ((mono && g_mono_adv > cur_width) ? ((g_mono_adv - cur_width) / 2) : 0), cur_y + y0);

                cur_x += (mono ? g_mono_adv : cur_width);

                prev_char = cur_char;
            }

            if (add_line) {
                /* Advance to next line. */
                g_cur_x = g_line_x;
                g_cur_y = cur_y + g_font_line_pixels;
            } else {
                g_cur_x = cur_x;
                g_cur_y = cur_y;
            }
        }

    }

    void PrintLine(const char *str) {
        return DrawString(str, true);
    }

    void PrintFormatLine(const char *format, ...) {
        char char_buf[0x400];

        std::va_list va_arg;
        va_start(va_arg, format);
        std::vsnprintf(char_buf, sizeof(char_buf), format, va_arg);
        va_end(va_arg);

        PrintLine(char_buf);
    }

    void Print(const char *str) {
        return DrawString(str, false);
    }

    void PrintFormat(const char *format, ...) {
        char char_buf[0x400];

        std::va_list va_arg;
        va_start(va_arg, format);
        std::vsnprintf(char_buf, sizeof(char_buf), format, va_arg);
        va_end(va_arg);

        Print(char_buf);
    }

    void PrintMonospaceU64(u64 x) {
        char char_buf[0x400];
        std::snprintf(char_buf, sizeof(char_buf), "%016lX", x);

        DrawString(char_buf, false, true);
    }

    void PrintMonospaceU32(u32 x) {
        char char_buf[0x400];
        std::snprintf(char_buf, sizeof(char_buf), "%08X", x);

        DrawString(char_buf, false, true);
    }

    void PrintMonospaceBlank(u32 width) {
        char char_buf[0x400] = {0};
        std::memset(char_buf, ' ', std::min(size_t(width), sizeof(char_buf)));

        DrawString(char_buf, false, true);
    }

    void SetFontColor(u16 color) {
        g_font_color = color;
    }

    void SetPosition(u32 x, u32 y) {
        g_line_x = x;
        g_cur_x = x;
        g_cur_y = y;
    }

    u32 GetX() {
        return g_cur_x;
    }

    u32 GetY() {
        return g_cur_y;
    }

    void SetFontSize(float fsz) {
        g_font_size = stbtt_ScaleForPixelHeight(&g_stb_font, fsz * 1.375);

        int ascent;
        stbtt_GetFontVMetrics(&g_stb_font, &ascent,0,0);
        g_font_line_pixels = ascent * g_font_size * 1.125;

        int adv_width, left_side_bearing;
        stbtt_GetCodepointHMetrics(&g_stb_font, 'A', &adv_width, &left_side_bearing);

        g_mono_adv = adv_width * g_font_size;
    }

    void AddSpacingLines(float num_lines) {
        g_cur_x = g_line_x;
        g_cur_y += static_cast<u32>(g_font_line_pixels * num_lines);
    }

    void ConfigureFontFramebuffer(u16 *fb, u32 (*unswizzle_func)(u32, u32)) {
        g_frame_buffer = fb;
        g_unswizzle_func = unswizzle_func;
    }

    Result InitializeSharedFont() {
        R_TRY(plGetSharedFontByType(&g_font, PlSharedFontType_Standard));

        u8 *font_buffer = reinterpret_cast<u8 *>(g_font.address);
        stbtt_InitFont(&g_stb_font, font_buffer, stbtt_GetFontOffsetForIndex(font_buffer, 0));

        SetFontSize(16.0f);
        return ResultSuccess();
    }

}
