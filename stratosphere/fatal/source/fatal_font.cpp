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
#include <ft2build.h>
#include FT_FREETYPE_H

#include "fatal_config.hpp"
#include "fatal_font.hpp"

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
        float g_font_size = 16.0f;
        u32 g_line_x = 0, g_cur_x = 0, g_cur_y = 0;

        u32 g_mono_adv = 0;

        PlFontData g_font;
        PlFontData g_fonts[PlSharedFontType_Total];

        FT_Library g_library;
        FT_Face g_face;
        FT_Error g_ft_err = 0;

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

        void DrawGlyph(FT_Bitmap *bitmap, u32 x, u32 y) {
            u8* imageptr = bitmap->buffer;

            if (bitmap->pixel_mode!=FT_PIXEL_MODE_GRAY) return;

            for (u32 tmpy = 0; tmpy < bitmap->rows; tmpy++) {
                for (u32 tmpx = 0; tmpx < bitmap->width; tmpx++) {
                    /* Implement very simple blending, as the bitmap value is an alpha value. */
                    u16 *ptr = &g_frame_buffer[g_unswizzle_func(x + tmpx, y + tmpy)];
                    *ptr = Blend(g_font_color, *ptr, imageptr[tmpx]);
                }
                imageptr += bitmap->pitch;
            }
        }

        void DrawString(const char *str, bool add_line, bool mono = false) {
            FT_UInt glyph_index;
            FT_GlyphSlot slot = g_face->glyph;

            const size_t len = strlen(str);

            u32 cur_x = g_cur_x, cur_y = g_cur_y;
            ON_SCOPE_EXIT {
                if (add_line) {
                    /* Advance to next line. */
                    g_cur_x = g_line_x;
                    g_cur_y = cur_y + (g_face->size->metrics.height >> 6);
                } else {
                    g_cur_x = cur_x;
                    g_cur_y = cur_y;
                }
            };

            for (u32 i = 0; i < len; ) {
                u32 cur_char;
                ssize_t unit_count = decode_utf8(&cur_char, reinterpret_cast<const u8 *>(&str[i]));
                if (unit_count <= 0) break;
                i += unit_count;

                if (cur_char == '\n') {
                    cur_x = g_line_x;
                    cur_y += g_face->size->metrics.height >> 6;
                    continue;
                }

                glyph_index = FT_Get_Char_Index(g_face, cur_char);

                g_ft_err = FT_Load_Glyph(g_face, glyph_index, FT_LOAD_DEFAULT);

                if (g_ft_err == 0) {
                    g_ft_err = FT_Render_Glyph(g_face->glyph, FT_RENDER_MODE_NORMAL);
                }

                if (g_ft_err) {
                    return;
                }

                DrawGlyph(&slot->bitmap, cur_x + slot->bitmap_left + ((mono && g_mono_adv > slot->advance.x) ? ((g_mono_adv - slot->advance.x) >> 7) : 0), cur_y - slot->bitmap_top);

                cur_x += (mono ? g_mono_adv : slot->advance.x) >> 6;
                cur_y += slot->advance.y >> 6;
            }
        }
    }

    void PrintLine(const char *str) {
        return DrawString(str, true);
    }

    void PrintFormatLine(const char *format, ...) {
        char char_buf[0x400];

        va_list va_arg;
        va_start(va_arg, format);
        vsnprintf(char_buf, sizeof(char_buf), format, va_arg);
        va_end(va_arg);

        PrintLine(char_buf);
    }

    void Print(const char *str) {
        return DrawString(str, false);
    }

    void PrintFormat(const char *format, ...) {
        char char_buf[0x400];

        va_list va_arg;
        va_start(va_arg, format);
        vsnprintf(char_buf, sizeof(char_buf), format, va_arg);
        va_end(va_arg);

        Print(char_buf);
    }

    void PrintMonospaceU64(u64 x) {
        char char_buf[0x400];
        snprintf(char_buf, sizeof(char_buf), "%016lX", x);

        DrawString(char_buf, false, true);
    }

    void PrintMonospaceU32(u32 x) {
        char char_buf[0x400];
        snprintf(char_buf, sizeof(char_buf), "%08X", x);

        DrawString(char_buf, false, true);
    }

    void PrintMonospaceBlank(u32 width) {
        char char_buf[0x400] = {0};
        for (size_t i = 0; i < width && i < sizeof(char_buf); i++) {
            char_buf[i] = ' ';
        }

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
        g_font_size = fsz;
        g_ft_err = FT_Set_Char_Size(g_face, 0, static_cast<u32>(g_font_size * 64.0f), 96, 96);

        g_ft_err = FT_Load_Glyph(g_face, FT_Get_Char_Index(g_face, 'A'), FT_LOAD_DEFAULT);

        if (g_ft_err == 0) {
            g_ft_err = FT_Render_Glyph(g_face->glyph, FT_RENDER_MODE_NORMAL);
        }
        if (g_ft_err == 0) {
            g_mono_adv = g_face->glyph->advance.x;
        }
    }

    void AddSpacingLines(float num_lines) {
        g_cur_x = g_line_x;
        g_cur_y += static_cast<u32>((static_cast<float>(g_face->size->metrics.height) * num_lines) / 64.0f);
    }

    void ConfigureFontFramebuffer(u16 *fb, u32 (*unswizzle_func)(u32, u32)) {
        g_frame_buffer = fb;
        g_unswizzle_func = unswizzle_func;
    }

    Result InitializeSharedFont() {
        s32 total_fonts = 0;

        R_TRY(plGetSharedFont(GetFatalConfig().GetLanguageCode(), g_fonts, PlSharedFontType_Total, &total_fonts));
        R_TRY(plGetSharedFontByType(&g_font, PlSharedFontType_Standard));

        g_ft_err = FT_Init_FreeType(&g_library);
        if (g_ft_err) return g_ft_err;

        g_ft_err = FT_New_Memory_Face(g_library, reinterpret_cast<const FT_Byte *>(g_font.address), g_font.size, 0, &g_face);
        if (g_ft_err) return g_ft_err;

        SetFontSize(g_font_size);
        return g_ft_err;
    }

}
