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
#include <stratosphere.hpp>
#include <strings.h>
#include "boot_functions.hpp"
#include "boot_reboot_manager.hpp"
#include "fusee-primary_bin.h"

static u8 g_work_page[0x1000] __attribute__ ((aligned (0x1000)));

static void ClearIram() {
    /* Make page FFs. */
    memset(g_work_page, 0xFF, sizeof(g_work_page));

    /* Overwrite all of IRAM with FFs. */
    for (size_t ofs = 0; ofs < IRAM_SIZE; ofs += sizeof(g_work_page)) {
        CopyToIram(IRAM_BASE + ofs, g_work_page, sizeof(g_work_page));
    }
}

static void DoRebootToPayload() {
    /* Ensure clean IRAM state. */
    ClearIram();

    /* Copy in payload. */
    for (size_t ofs = 0; ofs < fusee_primary_bin_size; ofs += 0x1000) {
        std::memcpy(g_work_page, &fusee_primary_bin[ofs], std::min(static_cast<size_t>(fusee_primary_bin_size - ofs), size_t(0x1000)));
        CopyToIram(IRAM_PAYLOAD_BASE + ofs, g_work_page, 0x1000);
    }

    RebootToIramPayload();
}

Result BootRebootManager::PerformReboot() {
    DoRebootToPayload();
    return ResultSuccess;
}

void BootRebootManager::RebootForFatalError(AtmosphereFatalErrorContext *ctx) {
    /* Ensure clean IRAM state. */
    ClearIram();

    /* Copy in payload. */
    for (size_t ofs = 0; ofs < fusee_primary_bin_size; ofs += 0x1000) {
        std::memcpy(g_work_page, &fusee_primary_bin[ofs], std::min(static_cast<size_t>(fusee_primary_bin_size - ofs), size_t(0x1000)));
        CopyToIram(IRAM_PAYLOAD_BASE + ofs, g_work_page, 0x1000);
    }

    std::memset(g_work_page, 0xCC, sizeof(g_work_page));
    std::memcpy(g_work_page, ctx, sizeof(*ctx));
    CopyToIram(IRAM_PAYLOAD_BASE + IRAM_PAYLOAD_MAX_SIZE, g_work_page, sizeof(g_work_page));

    RebootToIramPayload();
}

void Boot::RebootSystem() {
    if (R_FAILED(BootRebootManager::PerformReboot())) {
        std::abort();
    }
}