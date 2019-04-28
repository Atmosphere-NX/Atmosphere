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

#include <switch.h>

#include <atmosphere/version.h>

#include "fatal_task_screen.hpp"
#include "fatal_config.hpp"
#include "fatal_font.hpp"
#include "ams_logo.hpp"

static constexpr u32 FatalScreenWidth = 1280;
static constexpr u32 FatalScreenHeight = 720;
static constexpr u32 FatalScreenBpp = 2;

static constexpr u32 FatalScreenWidthAlignedBytes = (FatalScreenWidth * FatalScreenBpp + 63) & ~63;
static constexpr u32 FatalScreenWidthAligned = FatalScreenWidthAlignedBytes / FatalScreenBpp;

u32 GetPixelOffset(uint32_t x, uint32_t y)
{
    u32 tmp_pos;

    tmp_pos = ((y & 127) / 16) + (x/32*8) + ((y/16/8)*(((FatalScreenWidthAligned/2)/16*8)));
    tmp_pos *= 16*16 * 4;

    tmp_pos += ((y%16)/8)*512 + ((x%32)/16)*256 + ((y%8)/2)*64 + ((x%16)/8)*32 + (y%2)*16 + (x%8)*2;//This line is a modified version of code from the Tegra X1 datasheet.

    return tmp_pos / 2;
}

Result ShowFatalTask::SetupDisplayInternal() {
    Result rc;
    ViDisplay display;
    /* Try to open the display. */
    if (R_FAILED((rc = viOpenDisplay("Internal", &display)))) {
        if (rc == ResultViNotFound) {
            return ResultSuccess;
        } else {
            return rc;
        }
    }
    /* Guarantee we close the display. */
    ON_SCOPE_EXIT { viCloseDisplay(&display); };

    /* Turn on the screen. */
    if (R_FAILED((rc = viSetDisplayPowerState(&display, ViPowerState_On)))) {
        return rc;
    }

    /* Set alpha to 1.0f. */
    if (R_FAILED((rc = viSetDisplayAlpha(&display, 1.0f)))) {
        return rc;
    }

    return rc;
}

Result ShowFatalTask::SetupDisplayExternal() {
    Result rc;
    ViDisplay display;
    /* Try to open the display. */
    if (R_FAILED((rc = viOpenDisplay("External", &display)))) {
        if (rc == ResultViNotFound) {
            return ResultSuccess;
        } else {
            return rc;
        }
    }
    /* Guarantee we close the display. */
    ON_SCOPE_EXIT { viCloseDisplay(&display); };

    /* Set alpha to 1.0f. */
    if (R_FAILED((rc = viSetDisplayAlpha(&display, 1.0f)))) {
        return rc;
    }

    return rc;
}

Result ShowFatalTask::PrepareScreenForDrawing() {
    Result rc = ResultSuccess;

    /* Connect to vi. */
    if (R_FAILED((rc = viInitialize(ViServiceType_Manager)))) {
        return rc;
    }

    /* Close other content. */
    viSetContentVisibility(false);

    /* Setup the two displays. */
    if (R_FAILED((rc = SetupDisplayInternal())) || R_FAILED((rc = SetupDisplayExternal()))) {
        return rc;
    }

    /* Open the default display. */
    if (R_FAILED((rc = viOpenDefaultDisplay(&this->display)))) {
        return rc;
    }

    /* Reset the display magnification to its default value. */
    u32 display_width, display_height;
    if (R_FAILED((rc = viGetDisplayLogicalResolution(&this->display, &display_width, &display_height)))) {
        return rc;
    }

    /* viSetDisplayMagnification was added in 3.0.0. */
    if (GetRuntimeFirmwareVersion() >= FirmwareVersion_300) {
        if (R_FAILED((rc = viSetDisplayMagnification(&this->display, 0, 0, display_width, display_height)))) {
            return rc;
        }
    }

    /* Create layer to draw to. */
    if (R_FAILED((rc = viCreateLayer(&this->display, &this->layer)))) {
        return rc;
    }

    /* Setup the layer. */
    {
        /* Display a layer of 1280 x 720 at 1.5x magnification */
        /* NOTE: N uses 2 (770x400) RGBA4444 buffers (tiled buffer + linear). */
        /* We use a single 1280x720 tiled RGB565 buffer. */
        constexpr u32 raw_width = FatalScreenWidth;
        constexpr u32 raw_height = FatalScreenHeight;
        constexpr u32 layer_width = ((raw_width) * 3) / 2;
        constexpr u32 layer_height = ((raw_height) * 3) / 2;

        const float layer_x = static_cast<float>((display_width - layer_width) / 2);
        const float layer_y = static_cast<float>((display_height - layer_height) / 2);
        u64 layer_z;

        if (R_FAILED((rc = viSetLayerSize(&this->layer, layer_width, layer_height)))) {
            return rc;
        }

        /* Set the layer's Z at display maximum, to be above everything else .*/
        /* NOTE: Fatal hardcodes 100 here. */
        if (R_SUCCEEDED((rc = viGetDisplayMaximumZ(&this->display, &layer_z)))) {
            if (R_FAILED((rc = viSetLayerZ(&this->layer, layer_z)))) {
                return rc;
            }
        }

        /* Center the layer in the screen. */
        if (R_FAILED((rc = viSetLayerPosition(&this->layer, layer_x, layer_y)))) {
            return rc;
        }

        /* Create framebuffer. */
        if (R_FAILED(rc = nwindowCreateFromLayer(&this->win, &this->layer))) {
            return rc;
        }
        if (R_FAILED(rc = framebufferCreate(&this->fb, &this->win, raw_width, raw_height, PIXEL_FORMAT_RGB_565, 1))) {
            return rc;
        }
    }


    return rc;
}

Result ShowFatalTask::ShowFatal() {
    Result rc = ResultSuccess;
    const FatalConfig *config = GetFatalConfig();

    DoWithSmSession([&]() {
        rc = PrepareScreenForDrawing();
    });
    if (R_FAILED(rc)) {
        *(volatile u32 *)(0xCAFEBABE) = rc;
        return rc;
    }

    /* Dequeue a buffer. */
    u16 *tiled_buf = reinterpret_cast<u16 *>(framebufferBegin(&this->fb, NULL));
    if (tiled_buf == nullptr) {
        return ResultFatalNullGraphicsBuffer;
    }

    /* Let the font manager know about our framebuffer. */
    FontManager::ConfigureFontFramebuffer(tiled_buf, GetPixelOffset);
    FontManager::SetFontColor(0xFFFF);

    /* Draw a background. */
    for (size_t i = 0; i < this->fb.fb_size / sizeof(*tiled_buf); i++) {
        tiled_buf[i] = 0x39C9;
    }

    /* Draw the atmosphere logo in the bottom right corner. */
    for (size_t y = 0; y < AMS_LOGO_HEIGHT; y++) {
        for (size_t x = 0; x < AMS_LOGO_WIDTH; x++) {
            tiled_buf[GetPixelOffset(FatalScreenWidth - AMS_LOGO_WIDTH - 32 + x, 32 + y)] = AMS_LOGO_BIN[y * AMS_LOGO_WIDTH + x];
        }
    }

    /* TODO: Actually draw meaningful shit here. */
    FontManager::SetPosition(32, 64);
    FontManager::SetFontSize(16.0f);
    FontManager::PrintFormat(config->error_msg, R_MODULE(this->ctx->error_code), R_DESCRIPTION(this->ctx->error_code), this->ctx->error_code);
    FontManager::AddSpacingLines(0.5f);
    FontManager::PrintFormatLine("Title: %016lX", this->title_id);
    FontManager::AddSpacingLines(0.5f);
    FontManager::PrintFormatLine(u8"Firmware: %s (Atmosphère %u.%u.%u-%s)", GetFatalConfig()->firmware_version.display_version, CURRENT_ATMOSPHERE_VERSION, GetAtmosphereGitRevision());
    FontManager::AddSpacingLines(1.5f);
    if (this->ctx->error_code != ResultAtmosphereVersionMismatch) {
        FontManager::Print(config->error_desc);
    } else {
        /* Print a special message for atmosphere version mismatch. */
        FontManager::Print(u8"Atmosphère version mismatch detected.\n\n"
                           u8"Please press the POWER Button to restart the console normally, or a VOL button\n"
                           u8"to reboot to a payload (or RCM, if none is present). If you are unable to\n"
                           u8"restart the console, hold the POWER Button for 12 seconds to turn the console off.\n\n"
                           u8"Please ensure that all Atmosphère components are updated.\n"
                           u8"github.com/Atmosphere-NX/Atmosphere/releases\n");
    }

    /* Add a line. */
    for (size_t x = 32; x < FatalScreenWidth - 32; x++) {
        tiled_buf[GetPixelOffset(x, FontManager::GetY())] = 0xFFFF;
    }


    FontManager::AddSpacingLines(1.5f);

    u32 backtrace_y = FontManager::GetY();
    u32 backtrace_x = 0;

    /* Print GPRs. */
    FontManager::SetFontSize(14.0f);
    FontManager::Print("General Purpose Registers      ");
    {
        FontManager::SetPosition(FontManager::GetX() + 2, FontManager::GetY());
        u32 x = FontManager::GetX();
        FontManager::Print("PC: ");
        FontManager::SetPosition(x + 47, FontManager::GetY());
    }
    if (this->ctx->cpu_ctx.is_aarch32) {
        FontManager::PrintMonospaceU32(this->ctx->cpu_ctx.aarch32_ctx.pc);
    } else {
        FontManager::PrintMonospaceU64(this->ctx->cpu_ctx.aarch64_ctx.pc);
    }
    FontManager::PrintLine("");
    FontManager::SetPosition(32, FontManager::GetY());
    FontManager::AddSpacingLines(0.5f);
    if (this->ctx->cpu_ctx.is_aarch32) {
        for (size_t i = 0; i < (NumAarch32Gprs / 2); i++) {
            u32 x = FontManager::GetX();
            FontManager::PrintFormat("%s:", Aarch32GprNames[i]);
            FontManager::SetPosition(x + 47, FontManager::GetY());
            if (this->ctx->has_gprs[i]) {
                FontManager::PrintMonospaceU32(this->ctx->cpu_ctx.aarch32_ctx.r[i]);
            } else {
                FontManager::PrintMonospaceBlank(8);
            }
            FontManager::Print("  ");
            x = FontManager::GetX();
            FontManager::PrintFormat("%s:", Aarch32GprNames[i + (NumAarch32Gprs / 2)]);
            FontManager::SetPosition(x + 47, FontManager::GetY());
            if (this->ctx->has_gprs[i + (NumAarch32Gprs / 2)]) {
                FontManager::PrintMonospaceU32(this->ctx->cpu_ctx.aarch32_ctx.r[i + (NumAarch32Gprs / 2)]);
            } else {
                FontManager::PrintMonospaceBlank(8);
            }

            if (i == (NumAarch32Gprs / 2) - 1) {
                FontManager::Print("    ");
                backtrace_x = FontManager::GetX();
            }

            FontManager::PrintLine("");
            FontManager::SetPosition(32, FontManager::GetY());
        }
    } else {
        for (size_t i = 0; i < NumAarch64Gprs / 2; i++) {
            u32 x = FontManager::GetX();
            FontManager::PrintFormat("%s:", Aarch64GprNames[i]);
            FontManager::SetPosition(x + 47, FontManager::GetY());
            if (this->ctx->has_gprs[i]) {
                FontManager::PrintMonospaceU64(this->ctx->cpu_ctx.aarch64_ctx.x[i]);
            } else {
                FontManager::PrintMonospaceBlank(16);
            }
            FontManager::Print("  ");
            x = FontManager::GetX();
            FontManager::PrintFormat("%s:", Aarch64GprNames[i + (NumAarch64Gprs / 2)]);
            FontManager::SetPosition(x + 47, FontManager::GetY());
            if (this->ctx->has_gprs[i + (NumAarch64Gprs / 2)]) {
                FontManager::PrintMonospaceU64(this->ctx->cpu_ctx.aarch64_ctx.x[i + (NumAarch64Gprs / 2)]);
            } else {
                FontManager::PrintMonospaceBlank(16);
            }

            if (i == (NumAarch64Gprs / 2) - 1) {
                FontManager::Print("    ");
                backtrace_x = FontManager::GetX();
            }

            FontManager::PrintLine("");
            FontManager::SetPosition(32, FontManager::GetY());
        }
    }

    /* Print Backtrace. */
    u32 bt_size;
    if (this->ctx->cpu_ctx.is_aarch32) {
        bt_size = this->ctx->cpu_ctx.aarch32_ctx.stack_trace_size;
    } else {
        bt_size = this->ctx->cpu_ctx.aarch64_ctx.stack_trace_size;
    }


    FontManager::SetPosition(backtrace_x, backtrace_y);
    if (bt_size == 0) {
        if (this->ctx->cpu_ctx.is_aarch32) {
            FontManager::Print("Start Address: ");
            FontManager::PrintMonospaceU32(this->ctx->cpu_ctx.aarch32_ctx.start_address);
            FontManager::PrintLine("");
        } else {
            FontManager::Print("Start Address: ");
            FontManager::PrintMonospaceU64(this->ctx->cpu_ctx.aarch64_ctx.start_address);
            FontManager::PrintLine("");
        }
    } else {
        if (this->ctx->cpu_ctx.is_aarch32) {
            FontManager::Print("Backtrace - Start Address: ");
            FontManager::PrintMonospaceU32(this->ctx->cpu_ctx.aarch32_ctx.start_address);
            FontManager::PrintLine("");
            FontManager::AddSpacingLines(0.5f);
            for (u32 i = 0; i < Aarch32CpuContext::MaxStackTraceDepth / 2; i++) {
                u32 bt_cur = 0, bt_next = 0;
                if (i < this->ctx->cpu_ctx.aarch32_ctx.stack_trace_size) {
                    bt_cur = this->ctx->cpu_ctx.aarch32_ctx.stack_trace[i];
                }
                if (i + Aarch32CpuContext::MaxStackTraceDepth / 2 < this->ctx->cpu_ctx.aarch32_ctx.stack_trace_size) {
                    bt_next = this->ctx->cpu_ctx.aarch32_ctx.stack_trace[i + Aarch32CpuContext::MaxStackTraceDepth / 2];
                }

                if (i < this->ctx->cpu_ctx.aarch32_ctx.stack_trace_size) {
                    u32 x = FontManager::GetX();
                    FontManager::PrintFormat("BT[%02d]: ", i);
                    FontManager::SetPosition(x + 72, FontManager::GetY());
                    FontManager::PrintMonospaceU32(bt_cur);
                    FontManager::Print("  ");
                }

                if (i + Aarch32CpuContext::MaxStackTraceDepth / 2 < this->ctx->cpu_ctx.aarch32_ctx.stack_trace_size) {
                    u32 x = FontManager::GetX();
                    FontManager::PrintFormat("BT[%02d]: ", i + Aarch32CpuContext::MaxStackTraceDepth / 2);
                    FontManager::SetPosition(x + 72, FontManager::GetY());
                    FontManager::PrintMonospaceU32(bt_next);
                }

                FontManager::PrintLine("");
                FontManager::SetPosition(backtrace_x, FontManager::GetY());
            }
        } else {
            FontManager::Print("Backtrace - Start Address: ");
            FontManager::PrintMonospaceU64(this->ctx->cpu_ctx.aarch64_ctx.start_address);
            FontManager::PrintLine("");
            FontManager::AddSpacingLines(0.5f);
            for (u32 i = 0; i < Aarch64CpuContext::MaxStackTraceDepth / 2; i++) {
                u64 bt_cur = 0, bt_next = 0;
                if (i < this->ctx->cpu_ctx.aarch64_ctx.stack_trace_size) {
                    bt_cur = this->ctx->cpu_ctx.aarch64_ctx.stack_trace[i];
                }
                if (i + Aarch64CpuContext::MaxStackTraceDepth / 2 < this->ctx->cpu_ctx.aarch64_ctx.stack_trace_size) {
                    bt_next = this->ctx->cpu_ctx.aarch64_ctx.stack_trace[i + Aarch64CpuContext::MaxStackTraceDepth / 2];
                }

                if (i < this->ctx->cpu_ctx.aarch64_ctx.stack_trace_size) {
                    u32 x = FontManager::GetX();
                    FontManager::PrintFormat("BT[%02d]: ", i);
                    FontManager::SetPosition(x + 72, FontManager::GetY());
                    FontManager::PrintMonospaceU64(bt_cur);
                    FontManager::Print("  ");
                }

                if (i + Aarch64CpuContext::MaxStackTraceDepth / 2 < this->ctx->cpu_ctx.aarch64_ctx.stack_trace_size) {
                    u32 x = FontManager::GetX();
                    FontManager::PrintFormat("BT[%02d]: ", i + Aarch64CpuContext::MaxStackTraceDepth / 2);
                    FontManager::SetPosition(x + 72, FontManager::GetY());
                    FontManager::PrintMonospaceU64(bt_next);
                }

                FontManager::PrintLine("");
                FontManager::SetPosition(backtrace_x, FontManager::GetY());
            }
        }
    }


    /* Enqueue the buffer. */
    framebufferEnd(&fb);

    return rc;
}

Result ShowFatalTask::Run() {
    /* Don't show the fatal error screen until we've verified the battery is okay. */
    eventWait(this->battery_event, U64_MAX);

    return ShowFatal();
}

void BacklightControlTask::TurnOnBacklight() {
    lblSwitchBacklightOn(0);
}

Result BacklightControlTask::Run() {
    TurnOnBacklight();
    return ResultSuccess;
}
