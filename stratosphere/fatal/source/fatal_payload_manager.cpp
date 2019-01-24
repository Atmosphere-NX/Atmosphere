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
#include <stratosphere.hpp>
#include <string.h>
#include "fatal_types.hpp"
#include "fatal_payload_manager.hpp"

/* TODO: Find a way to pre-populate this with the contents of fusee-primary. */
static u8 g_reboot_payload[IRAM_PAYLOAD_MAX_SIZE] __attribute__ ((aligned (0x1000)));
static u8 g_work_page[0x1000] __attribute__ ((aligned (0x1000)));
static bool g_payload_loaded = false;

void FatalPayloadManager::LoadPayloadFromSdCard() {
    FILE *f = fopen("sdmc:/atmosphere/reboot_payload.bin", "rb");
    if (f == NULL) {
        return;
    }
    ON_SCOPE_EXIT { fclose(f); };
    
    memset(g_reboot_payload, 0xFF, sizeof(g_reboot_payload));
    fread(g_reboot_payload, 1, IRAM_PAYLOAD_MAX_SIZE, f);
    g_payload_loaded = true;
}

static void ClearIram() {
    /* Make page FFs. */
    memset(g_work_page, 0xFF, sizeof(g_work_page));
    
    /* Overwrite all of IRAM with FFs. */
    for (size_t ofs = 0; ofs < IRAM_PAYLOAD_MAX_SIZE; ofs += sizeof(g_work_page)) {
        CopyToIram(IRAM_PAYLOAD_BASE + ofs, g_work_page, sizeof(g_work_page));
    }
}

void FatalPayloadManager::RebootToPayload() {
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