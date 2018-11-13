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
#include "fatal_task_screen.hpp"
#include "fatal_config.hpp"

Result ShowFatalTask::SetupDisplayInternal() {
    Result rc;
    ViDisplay display;
    /* Try to open the display. */
    if (R_FAILED((rc = viOpenDisplay("Internal", &display)))) {
        if (rc == 0xE72) {
            return 0;
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
        if (rc == 0xE72) {
            return 0;
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
    Result rc = 0;
    
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
    if (R_FAILED((rc = viSetDisplayMagnification(&this->display, 0, 0, display_width, display_height)))) {
        return rc;
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
        constexpr u32 raw_width = 1280;
        constexpr u32 raw_height = 720;
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
    Result rc = 0;

    if (R_FAILED((rc = PrepareScreenForDrawing()))) {
        *(volatile u32 *)(0xCAFEBABE) = rc;
        return rc;
    }
    
    /* Dequeue a buffer. */
    u16 *tiled_buf = reinterpret_cast<u16 *>(framebufferBegin(&this->fb, NULL));
    if (tiled_buf == nullptr) {
        return FatalResult_NullGfxBuffer;
    }
    
    /* Draw a background. */
    for (size_t i = 0; i < this->fb.fb_size / sizeof(*tiled_buf); i++) {
        tiled_buf[i] = 0x39C9;
    }
    
    /* TODO: Actually draw meaningful shit here. */
    
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
    return 0;
}
