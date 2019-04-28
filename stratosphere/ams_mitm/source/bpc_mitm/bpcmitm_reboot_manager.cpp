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
#include "bpcmitm_reboot_manager.hpp"
#include "../utils.hpp"

/* TODO: Find a way to pre-populate this with the contents of fusee-primary. */
static u8 g_reboot_payload[IRAM_PAYLOAD_MAX_SIZE] __attribute__ ((aligned (0x1000)));
static u8 g_work_page[0x1000] __attribute__ ((aligned (0x1000)));
static bool g_payload_loaded = false;
static BpcRebootType g_reboot_type = BpcRebootType::ToPayload;

void BpcRebootManager::Initialize() {
    /* Open payload file. */
    FsFile payload_file;
    if (R_FAILED(Utils::OpenSdFile("/atmosphere/reboot_payload.bin", FS_OPEN_READ, &payload_file))) {
        return;
    }
    ON_SCOPE_EXIT { fsFileClose(&payload_file); };
    
    /* Clear payload buffer */
    std::memset(g_reboot_payload, 0xFF, sizeof(g_reboot_payload));
    
    /* Read payload file. */
    size_t actual_size;
    fsFileRead(&payload_file, 0, g_reboot_payload, IRAM_PAYLOAD_MAX_SIZE, &actual_size);
    
    g_payload_loaded = true;
    
    /* Figure out what kind of reboot we're gonna be doing. */
    {
        char reboot_type[0x40] = {0};
        u64 reboot_type_size = 0;
        if (R_SUCCEEDED(Utils::GetSettingsItemValue("atmosphere", "power_menu_reboot_function", reboot_type, sizeof(reboot_type)-1, &reboot_type_size))) {
            if (strcasecmp(reboot_type, "stock") == 0 || strcasecmp(reboot_type, "normal") == 0 || strcasecmp(reboot_type, "standard") == 0) {
                g_reboot_type = BpcRebootType::Standard;
            } else if (strcasecmp(reboot_type, "rcm") == 0) {
                g_reboot_type = BpcRebootType::ToRcm;
            } else if (strcasecmp(reboot_type, "payload") == 0) {
                g_reboot_type = BpcRebootType::ToPayload;
            }
        }
    }
}

static void ClearIram() {
    /* Make page FFs. */
    memset(g_work_page, 0xFF, sizeof(g_work_page));
    
    /* Overwrite all of IRAM with FFs. */
    for (size_t ofs = 0; ofs < IRAM_SIZE; ofs += sizeof(g_work_page)) {
        CopyToIram(IRAM_BASE + ofs, g_work_page, sizeof(g_work_page));
    }
}

static void DoRebootToPayload() {
    /* If we don't actually have a payload loaded, just go to RCM. */
    if (!g_payload_loaded) {
        RebootToRcm();
    }
    
    /* Ensure clean IRAM state. */
    ClearIram();
    
    /* Copy in payload. */
    for (size_t ofs = 0; ofs < sizeof(g_reboot_payload); ofs += 0x1000) {
        CopyToIram(IRAM_PAYLOAD_BASE + ofs, &g_reboot_payload[ofs], 0x1000);
    }
    
    RebootToIramPayload();
}

Result BpcRebootManager::PerformReboot() {
    switch (g_reboot_type) {
        case BpcRebootType::Standard:
            return ResultAtmosphereMitmShouldForwardToSession;
        case BpcRebootType::ToRcm:
            RebootToRcm();
            return ResultSuccess;
        case BpcRebootType::ToPayload:
        default:
            DoRebootToPayload();
            return ResultSuccess;
    }
}

void BpcRebootManager::RebootForFatalError(AtmosphereFatalErrorContext *ctx) {
    /* Ensure clean IRAM state. */
    ClearIram();
    
    
    /* Copy in payload. */
    for (size_t ofs = 0; ofs < sizeof(g_reboot_payload); ofs += 0x1000) {
        CopyToIram(IRAM_PAYLOAD_BASE + ofs, &g_reboot_payload[ofs], 0x1000);
    }
    
    memcpy(g_work_page, ctx, sizeof(*ctx));
    CopyToIram(IRAM_PAYLOAD_BASE + IRAM_PAYLOAD_MAX_SIZE, g_work_page, sizeof(g_work_page));
    
    /* If we don't actually have a payload loaded, just go to RCM. */
    if (!g_payload_loaded) {
        RebootToRcm();
    }

    RebootToIramPayload();
}