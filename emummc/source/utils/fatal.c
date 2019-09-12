/*
 * Copyright (c) 2019 m4xw <m4x@m4xw.net>
 * Copyright (c) 2019 Atmosphere-NX
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

#include <string.h>
#include "fatal.h"

void __attribute__((noreturn)) fatal_abort(enum FatalReason abortReason)
{
    atmosphere_fatal_error_ctx error_ctx;
    memset(&error_ctx, 0, sizeof(atmosphere_fatal_error_ctx));

    // Basic error storage for Atmosphere
    // TODO: Maybe include a small reboot2payload stub?
    error_ctx.magic = ATMOSPHERE_REBOOT_TO_FATAL_MAGIC;
    error_ctx.title_id = 0x0100000000000000; // FS
    error_ctx.error_desc = abortReason;

    // Copy fatal context
    smcCopyToIram(ATMOSPHERE_FATAL_ERROR_ADDR, &error_ctx, sizeof(atmosphere_fatal_error_ctx));

    // Reboot to RCM
    smcRebootToRcm();

    while (true)
        ; // Should never be reached
}
