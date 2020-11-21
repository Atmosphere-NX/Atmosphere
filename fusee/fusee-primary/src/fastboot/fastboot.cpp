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

#include "fastboot.h"
#include "fastboot_gadget.h"

extern "C" {
#include "../lib/log.h"
#include "../timers.h"
#include "../utils.h"
#include "../btn.h"
}

static ams::fastboot::FastbootGadget fastboot_gadget((uint8_t*) 0xf0000000, 1024 * 1024 * 256); // 256 MiB download buffer reaches end of address space

extern "C" fastboot_return fastboot_enter(const bct0_t *bct0, bool force) {
    bool should_enter = force || bct0->fastboot_force_enable;

    if(should_enter) {
        log_setup_display();
    } else {
        if (bct0->fastboot_button_timeout > 0) {
            log_setup_display();

            uint32_t timeout = bct0->fastboot_button_timeout;
            uint32_t start_time = get_time_ms();
            uint32_t last_message_secs = 0;

            while (get_time_ms() - start_time < timeout) {
                uint32_t seconds_remaining = (start_time + timeout - get_time_ms() + 999) / 1000;
                if (seconds_remaining != last_message_secs) {
                    print((ScreenLogLevel) (SCREEN_LOG_LEVEL_NONE | SCREEN_LOG_LEVEL_NO_PREFIX), "\rPress volume up in %d seconds to enter fastboot.   ", seconds_remaining);
                    last_message_secs = seconds_remaining;
                }

                if (btn_read() & BTN_VOL_UP) {
                    should_enter = true;
                    break;
                }
            }

            print((ScreenLogLevel) (SCREEN_LOG_LEVEL_NONE | SCREEN_LOG_LEVEL_NO_PREFIX), "\n\n");

            if (!should_enter) {
                log_cleanup_display();
                return FASTBOOT_SKIPPED;
            }
        } else {
            /* We did not initialize the display, so don't need to clean it up here. */

            return FASTBOOT_SKIPPED;
        }
    }

    print(SCREEN_LOG_LEVEL_DEBUG, "Entering fastboot. (forced = %d)\n", force);
    ams::xusb::Initialize();
    print(SCREEN_LOG_LEVEL_DEBUG, "Finished initializing USB hardware.\n");
    ams::xusb::EnableDevice(fastboot_gadget);

    print((ScreenLogLevel) (SCREEN_LOG_LEVEL_NONE | SCREEN_LOG_LEVEL_NO_PREFIX), "Fastboot mode:\n");
    print((ScreenLogLevel) (SCREEN_LOG_LEVEL_NONE | SCREEN_LOG_LEVEL_NO_PREFIX), "-------------------------------\n");
    print((ScreenLogLevel) (SCREEN_LOG_LEVEL_NONE | SCREEN_LOG_LEVEL_NO_PREFIX), "Volume up: Reboot to RCM.\n");
    print((ScreenLogLevel) (SCREEN_LOG_LEVEL_NONE | SCREEN_LOG_LEVEL_NO_PREFIX), "Volume down: Boot normally.\n");
    print((ScreenLogLevel) (SCREEN_LOG_LEVEL_NONE | SCREEN_LOG_LEVEL_NO_PREFIX), "POWER: Reboot to fusee-primary.\n");

    fastboot_return r = fastboot_gadget.Run();

    log_cleanup_display();

    return r;
}
