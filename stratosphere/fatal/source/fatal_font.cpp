/*
 * Copyright (c) 2018 Atmosphère-NX
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
 

#include <switch.h>
#include "fatal_types.hpp"

#include <cstring>
#include <cstdlib>
#include <cstdio>

#include <ft2build.h>
#include FT_FREETYPE_H

#include "fatal_config.hpp"
#include "fatal_font.hpp"

static u16 *g_fb = nullptr;
static u32 (*g_unswizzle_func)(u32, u32) = nullptr;
static u16 g_font_color = 0xFFFF;
static float g_font_sz = 16.0f;
static u32 g_line_x = 0, g_cur_x = 0, g_cur_y = 0;

static u32 g_mono_adv = 0;

static PlFontData g_font;
static PlFontData g_fonts[PlSharedFontType_Total];
static FT_Library g_library;
static FT_Face g_face;
static FT_Error g_ft_err = 0;

static u16 Blend(u16 color, u16 bg, u8 alpha) {
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

static void DrawGlyph(FT_Bitmap *bitmap, u32 x, u32 y) {
    u8* imageptr = bitmap->buffer;

    if (bitmap->pixel_mode!=FT_PIXEL_MODE_GRAY) return;
    
    for (u32 tmpy = 0; tmpy < bitmap->rows; tmpy++) {
        for (u32 tmpx = 0; tmpx < bitmap->width; tmpx++) {
            /* Implement very simple blending, as the bitmap value is an alpha value. */
            u16 *ptr = &g_fb[g_unswizzle_func(x + tmpx, y + tmpy)];
            *ptr = Blend(g_font_color, *ptr, imageptr[tmpx]);
        }
        imageptr += bitmap->pitch;
    }
}

static void DrawString(const char *str, bool add_line, bool mono = false) {
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

void FontManager::PrintLine(const char *str) {
    return DrawString(str, true);
}

void FontManager::PrintFormatLine(const char *format, ...) {
    va_list va_arg;
    va_start(va_arg, format);
    
    char char_buf[0x400];
    vsnprintf(char_buf, sizeof(char_buf), format, va_arg);
    
    PrintLine(char_buf);
}

void FontManager::Print(const char *str) {
    return DrawString(str, false);
}

void FontManager::PrintFormat(const char *format, ...) {
    va_list va_arg;
    va_start(va_arg, format);
    
    char char_buf[0x400];
    vsnprintf(char_buf, sizeof(char_buf), format, va_arg);
    
    Print(char_buf);
}

void FontManager::PrintMonospaceU64(u64 x) {    
    char char_buf[0x400];
    snprintf(char_buf, sizeof(char_buf), "%016lX", x);
    
    DrawString(char_buf, false, true);
}

void FontManager::PrintMonospaceU32(u32 x) {    
    char char_buf[0x400];
    snprintf(char_buf, sizeof(char_buf), "%08X", x);
    
    DrawString(char_buf, false, true);
}

void FontManager::PrintMonospaceBlank(u32 width) {
    char char_buf[0x400] = {0};
    for (size_t i = 0; i < width && i < sizeof(char_buf); i++) {
        char_buf[i] = ' ';
    }
    
    DrawString(char_buf, false, true);
}

void FontManager::SetFontColor(u16 color) {
    g_font_color = color;
}

void FontManager::SetPosition(u32 x, u32 y) {
    g_line_x = x;
    g_cur_x = x;
    g_cur_y = y;
}

u32 FontManager::GetX() {
    return g_cur_x;
}

u32 FontManager::GetY() {
    return g_cur_y;
}

void FontManager::SetFontSize(float fsz) {
    g_font_sz = fsz;
    g_ft_err = FT_Set_Char_Size(g_face, 0, static_cast<u32>(g_font_sz * 64.0f), 96, 96);
        
    g_ft_err = FT_Load_Glyph(g_face, FT_Get_Char_Index(g_face, 'A'), FT_LOAD_DEFAULT);
    
    if (g_ft_err == 0) {
        g_ft_err = FT_Render_Glyph(g_face->glyph, FT_RENDER_MODE_NORMAL);
    }
    if (g_ft_err == 0) {
        g_mono_adv = g_face->glyph->advance.x;
    }
}

void FontManager::AddSpacingLines(float num_lines) {
    g_cur_x = g_line_x;
    g_cur_y += static_cast<u32>((static_cast<float>(g_face->size->metrics.height) * num_lines) / 64.0f);
}

void FontManager::ConfigureFontFramebuffer(u16 *fb, u32 (*unswizzle_func)(u32, u32)) {
    g_fb = fb;
    g_unswizzle_func = unswizzle_func;
}

Result FontManager::InitializeSharedFont() {
    Result rc;
    size_t total_fonts = 0;
    
    if (R_FAILED((rc = plGetSharedFont(GetFatalConfig()->language_code, g_fonts, PlSharedFontType_Total, &total_fonts)))) {
        return rc;
    }

    if (R_FAILED((rc = plGetSharedFontByType(&g_font, PlSharedFontType_Standard)))) {
        return rc;
    }
    
    g_ft_err = FT_Init_FreeType(&g_library);
    if (g_ft_err) return g_ft_err;
    
    g_ft_err = FT_New_Memory_Face(g_library, reinterpret_cast<const FT_Byte *>(g_font.address), g_font.size, 0, &g_face);
    if (g_ft_err) return g_ft_err;
    
    SetFontSize(g_font_sz);
    return g_ft_err;
}