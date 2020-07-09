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
#include "ui_util.hpp"
#include <cstdio>
#include <math.h>

namespace dbk {

    namespace {

        constexpr const char *SwitchStandardFont = "switch-standard";
        constexpr float WindowCornerRadius = 20.0f;
        constexpr float TextAreaCornerRadius = 10.0f;
        constexpr float ButtonCornerRaidus = 3.0f;

        NVGcolor GetSelectionRGB2(u64 ns) {
            /* Calculate the rgb values for the breathing colour effect. */
            const double t = static_cast<double>(ns) / 1'000'000'000.0d;
            const float d = -0.5 * cos(3.0f*t) + 0.5f;
            const int r2 = 83 + (float)(128 - 83) * (d * 0.7f + 0.3f);
            const int g2 = 71 + (float)(126 - 71) * (d * 0.7f + 0.3f);
            const int b2 = 185 + (float)(230 - 185) * (d * 0.7f + 0.3f);
            return nvgRGB(r2, g2, b2);
        }

    }

    void DrawStar(NVGcontext *vg, float x, float y, float width) {
        nvgBeginPath(vg);
        nvgEllipse(vg, x, y, width, width * 3.0f);
        nvgEllipse(vg, x, y, width * 3.0f, width);
        nvgFillColor(vg, nvgRGB(65, 71, 115));
        nvgFill(vg);
    }

    void DrawBackground(NVGcontext *vg, float w, float h) {
        /* Draw the background gradient. */
        const NVGpaint bg_paint = nvgLinearGradient(vg, w / 2.0f, 0, w / 2.0f, h + 20.0f, nvgRGB(20, 24, 50), nvgRGB(46, 57, 127));
        nvgBeginPath(vg);
        nvgRect(vg, 0, 0, w, h);
        nvgFillPaint(vg, bg_paint);
        nvgFill(vg);
    }

    void DrawWindow(NVGcontext *vg, const char *title, float x, float y, float w, float h) {
        /* Draw the window background. */
        const NVGpaint window_bg_paint = nvgLinearGradient(vg, x + w / 2.0f, y, x + w / 2.0f, y + h + h / 4.0f, nvgRGB(255, 255, 255), nvgRGB(188, 214, 234));
        nvgBeginPath(vg);
        nvgRoundedRect(vg, x, y, w, h, WindowCornerRadius);
        nvgFillPaint(vg, window_bg_paint);
        nvgFill(vg);

        /* Draw the shadow surrounding the window. */
        NVGpaint shadowPaint = nvgBoxGradient(vg, x, y + 2, w, h, WindowCornerRadius * 2, 10, nvgRGBA(0, 0, 0, 128), nvgRGBA(0, 0, 0, 0));
        nvgBeginPath(vg);
        nvgRect(vg, x - 10, y - 10, w + 20, h + 30);
        nvgRoundedRect(vg, x, y, w, h, WindowCornerRadius);
        nvgPathWinding(vg, NVG_HOLE);
        nvgFillPaint(vg, shadowPaint);
        nvgFill(vg);

        /* Setup the font. */
        nvgFontSize(vg, 32.0f);
        nvgFontFace(vg, SwitchStandardFont);
        nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, nvgRGB(0, 0, 0));

        /* Draw the title. */
        const float tw = nvgTextBounds(vg, 0, 0, title, nullptr, nullptr);
        nvgText(vg, x + w * 0.5f - tw * 0.5f, y + 40.0f, title, nullptr);
    }

    void DrawButton(NVGcontext *vg, const char *text, float x, float y, float w, float h, ButtonStyle style, u64 ns) {
        /* Fill the background if selected. */
        if (style == ButtonStyle::StandardSelected || style == ButtonStyle::FileSelectSelected) {
            NVGpaint bg_paint = nvgLinearGradient(vg, x, y + h / 2.0f, x + w, y + h / 2.0f, nvgRGB(83, 71, 185), GetSelectionRGB2(ns));
            nvgBeginPath(vg);
            nvgRoundedRect(vg, x, y, w, h, ButtonCornerRaidus);
            nvgFillPaint(vg, bg_paint);
            nvgFill(vg);
        }

        /* Draw the shadow surrounding the button. */
        if (style == ButtonStyle::Standard || style == ButtonStyle::StandardSelected || style == ButtonStyle::StandardDisabled || style == ButtonStyle::FileSelectSelected) {
            const unsigned char shadow_color = style == ButtonStyle::Standard ? 128 : 64;
            NVGpaint shadow_paint = nvgBoxGradient(vg, x, y, w, h, ButtonCornerRaidus, 5, nvgRGBA(0, 0, 0, shadow_color), nvgRGBA(0, 0, 0, 0));
            nvgBeginPath(vg);
            nvgRect(vg, x - 10, y - 10, w + 20, h + 30);
            nvgRoundedRect(vg, x, y, w, h, ButtonCornerRaidus);
            nvgPathWinding(vg, NVG_HOLE);
            nvgFillPaint(vg, shadow_paint);
            nvgFill(vg);
        }

        /* Setup the font. */
        nvgFontSize(vg, 20.0f);
        nvgFontFace(vg, SwitchStandardFont);
        nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);

        /* Set the text colour. */
        if (style == ButtonStyle::StandardSelected || style == ButtonStyle::FileSelectSelected) {
            nvgFillColor(vg, nvgRGB(255, 255, 255));
        } else {
            const unsigned char alpha = style == ButtonStyle::StandardDisabled ? 64 : 255;
            nvgFillColor(vg, nvgRGBA(0, 0, 0, alpha));
        }

        /* Draw the button text. */
        const float tw = nvgTextBounds(vg, 0, 0, text, nullptr, nullptr);

        if (style == ButtonStyle::Standard || style == ButtonStyle::StandardSelected || style == ButtonStyle::StandardDisabled) {
            nvgText(vg, x + w * 0.5f - tw * 0.5f, y + h * 0.5f, text, nullptr);
        } else {
            nvgText(vg, x + 10.0f, y + h * 0.5f, text, nullptr);
        }
    }

    void DrawTextBackground(NVGcontext *vg, float x, float y, float w, float h) {
        nvgBeginPath(vg);
        nvgRoundedRect(vg, x, y, w, h, TextAreaCornerRadius);
        nvgFillColor(vg, nvgRGBA(0, 0, 0, 16));
        nvgFill(vg);
    }

    void DrawText(NVGcontext *vg, float x, float y, float w, const char *text) {
        nvgFontSize(vg, 20.0f);
        nvgFontFace(vg, SwitchStandardFont);
        nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
        nvgFillColor(vg, nvgRGB(0, 0, 0));

        const float tw = nvgTextBounds(vg, 0, 0, text, nullptr, nullptr);
        nvgText(vg, x + w * 0.5f - tw * 0.5f, y, text, nullptr);
    }

    void DrawProgressText(NVGcontext *vg, float x, float y, float progress) {
        char progress_text[32] = {};
        snprintf(progress_text, sizeof(progress_text)-1, "%d%% complete", static_cast<int>(progress * 100.0f));

        nvgFontSize(vg, 24.0f);
        nvgFontFace(vg, SwitchStandardFont);
        nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, nvgRGB(0, 0, 0));
        nvgText(vg, x, y, progress_text, nullptr);
    }

    void DrawProgressBar(NVGcontext *vg, float x, float y, float w, float h, float progress) {
        /* Draw the progress bar background. */
        nvgBeginPath(vg);
        nvgRoundedRect(vg, x, y, w, h, WindowCornerRadius);
        nvgFillColor(vg, nvgRGBA(0, 0, 0, 128));
        nvgFill(vg);

        /* Draw the progress bar fill. */
        if (progress > 0.0f) {
            NVGpaint progress_fill_paint = nvgLinearGradient(vg, x, y + 0.5f * h, x + w, y + 0.5f * h, nvgRGB(83, 71, 185), nvgRGB(128, 126, 230));
            nvgBeginPath(vg);
            nvgRoundedRect(vg, x, y, WindowCornerRadius + (w - WindowCornerRadius) * progress, h, WindowCornerRadius);
            nvgFillPaint(vg, progress_fill_paint);
            nvgFill(vg);
        }
    }

    void DrawTextBlock(NVGcontext *vg, const char *text, float x, float y, float w, float h) {
        /* Save state and scissor. */
        nvgSave(vg);
        nvgScissor(vg, x, y, w, h);

        /* Configure the text. */
        nvgFontSize(vg, 18.0f);
        nvgFontFace(vg, SwitchStandardFont);
        nvgTextLineHeight(vg, 1.3f);
        nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
        nvgFillColor(vg, nvgRGB(0, 0, 0));

        /* Determine the bounds of the text box. */
        float bounds[4];
        nvgTextBoxBounds(vg, 0, 0, w, text, nullptr, bounds);

        /* Adjust the y to only show the last part of the text that fits. */
        float y_adjustment = 0.0f;
        if (bounds[3] > h) {
            y_adjustment = bounds[3] - h;
        }

        /* Draw the text box and restore state. */
        nvgTextBox(vg, x, y - y_adjustment, w, text, nullptr);
        nvgRestore(vg);
    }

}
