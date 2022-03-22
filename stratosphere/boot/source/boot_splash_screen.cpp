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
#include <stratosphere.hpp>
#include "boot_boot_reason.hpp"
#include "boot_display.hpp"
#include "boot_splash_screen.hpp"

namespace ams::boot {

    namespace {

/* Include splash screen into anonymous namespace. */
/* TODO: Compile-time switch for splash_screen_text.hpp? */
#include "boot_splash_screen_notext.inc"

    }

    void ShowSplashScreen() {
        const auto boot_reason = GetBootReason();
        if (boot_reason == spl::BootReason_AcOk || boot_reason == spl::BootReason_RtcAlarm2) {
            return;
        }

        InitializeDisplay();
        {
            /* Splash screen is shown for 2 seconds. */
            ShowDisplay(SplashScreenX, SplashScreenY, SplashScreenW, SplashScreenH, SplashScreen);
            os::SleepThread(TimeSpan::FromSeconds(2));
        }
        FinalizeDisplay();
    }

}

