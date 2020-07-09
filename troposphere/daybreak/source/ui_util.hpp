/*
 * Copyright (c) 2020 Adubbz
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
#include <nanovg.h>
#include <switch.h>

namespace dbk {

    enum class ButtonStyle {
        Standard,
        StandardSelected,
        StandardDisabled,
        FileSelect,
        FileSelectSelected,
    };

    void DrawStar(NVGcontext *vg, float x, float y, float width);
    void DrawBackground(NVGcontext *vg, float w, float h);
    void DrawWindow(NVGcontext *vg, const char *title, float x, float y, float w, float h);
    void DrawButton(NVGcontext *vg, const char *text, float x, float y, float w, float h, ButtonStyle style, u64 ns);
    void DrawTextBackground(NVGcontext *vg, float x, float y, float w, float h);
    void DrawText(NVGcontext *vg, float x, float y, float w, const char *text);
    void DrawProgressText(NVGcontext *vg, float x, float y, float progress);
    void DrawProgressBar(NVGcontext *vg, float x, float y, float w, float h, float progress);
    void DrawTextBlock(NVGcontext *vg, const char *text, float x, float y, float w, float h);

}
