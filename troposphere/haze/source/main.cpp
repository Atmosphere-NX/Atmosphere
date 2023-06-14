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
#include <haze.hpp>
#include <haze/console_main_loop.hpp>

int main(int argc, char **argv) {
    /* Lock exit so that cleanup is ensured. */
    HAZE_R_ABORT_UNLESS(appletLockExit());

    /* Load device firmware version and serial number. */
    HAZE_R_ABORT_UNLESS(haze::LoadDeviceProperties());

    /* Run the application. */
    haze::ConsoleMainLoop::RunApplication();

    /* Return to the loader. */
    appletUnlockExit();
    return 0;
}
