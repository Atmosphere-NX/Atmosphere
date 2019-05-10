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

#include "boot_functions.hpp"
#include "boot_splash_screen_notext.hpp"
/* TODO: Compile-time switch for splash_screen_text.hpp? */

void Boot::ShowSplashScreen() {
    const u32 boot_reason = Boot::GetBootReason();
    if (boot_reason == 1 || boot_reason == 4) {
        return;
    }

    Boot::InitializeDisplay();
    {
        /* Splash screen is shown for 2 seconds. */
        Boot::ShowDisplay(SplashScreenX, SplashScreenY, SplashScreenW, SplashScreenH, SplashScreen);
        svcSleepThread(2'000'000'000ul);
    }
    Boot::FinalizeDisplay();
}
