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
 
#include <cstring>
#include <switch.h>
#include <strings.h>
#include <vector>
#include <algorithm>

#include "ldr_registration.hpp"
#include "ldr_content_management.hpp"
#include "ldr_hid.hpp"


#include "ini.h"

static FsFileSystem g_CodeFileSystem = {0};
static FsFileSystem g_HblFileSystem = {0};

static std::vector<u64> g_created_titles;
static bool g_has_initialized_fs_dev = false;

/* Default to Key R, hold disables override, HBL at atmosphere/hbl.nsp. */
static bool g_mounted_hbl_nsp = false;
static char g_hbl_sd_path[FS_MAX_PATH+1] = "@Sdcard:/atmosphere/hbl.nsp\x00";
static u64 g_override_key_combination = KEY_R;
static bool g_override_by_default = true;
static u64 g_override_hbl_tid = 0x010000000000100D;

Result ContentManagement::MountCode(u64 tid, FsStorageId sid) {
    char path[FS_MAX_PATH] = {0};
    Result rc;
    
    /* We defer SD card mounting, so if relevant ensure it is mounted. */
    if (!g_has_initialized_fs_dev) {   
        TryMountSdCard();
    }
    
    if (ShouldOverrideContents() && R_SUCCEEDED(MountCodeNspOnSd(tid))) {
        return 0x0;
    }
        
    if (R_FAILED(rc = ResolveContentPath(path, tid, sid))) {
        return rc;
    }
    
    /* Fix up path. */
    for (unsigned int i = 0; i < FS_MAX_PATH && path[i] != '\x00'; i++) {
        if (path[i] == '\\') {
            path[i] = '/';
        }
    }
    
    /* Always re-initialize fsp-ldr, in case it's closed */
    if (R_FAILED(rc = fsldrInitialize())) {
        return rc;
    }
    
    if (R_FAILED(rc = fsldrOpenCodeFileSystem(tid, path, &g_CodeFileSystem))) {
        fsldrExit();
        return rc;
    }
    
    fsdevMountDevice("code", g_CodeFileSystem);
    TryMountHblNspOnSd();
    
    fsldrExit();
    return rc;
}

Result ContentManagement::UnmountCode() {
    if (g_mounted_hbl_nsp) {
        fsdevUnmountDevice("hbl");
        g_mounted_hbl_nsp = false;
    }
    fsdevUnmountDevice("code");
    return 0;
}


void ContentManagement::TryMountHblNspOnSd() {
    char path[FS_MAX_PATH + 1] = {0};
    strncpy(path, g_hbl_sd_path, FS_MAX_PATH);
    for (unsigned int i = 0; i < FS_MAX_PATH && path[i] != '\x00'; i++) {
        if (path[i] == '\\') {
            path[i] = '/';
        }
    }
    if (g_has_initialized_fs_dev && !g_mounted_hbl_nsp && R_SUCCEEDED(fsOpenFileSystemWithId(&g_HblFileSystem, 0, FsFileSystemType_ApplicationPackage, path))) {   
        fsdevMountDevice("hbl", g_HblFileSystem);
        g_mounted_hbl_nsp = true;
    }
}

Result ContentManagement::MountCodeNspOnSd(u64 tid) {
    char path[FS_MAX_PATH+1] = {0};
    snprintf(path, FS_MAX_PATH, "@Sdcard:/atmosphere/titles/%016lx/exefs.nsp", tid); 
    Result rc = fsOpenFileSystemWithId(&g_CodeFileSystem, 0, FsFileSystemType_ApplicationPackage, path);
    
    if (R_SUCCEEDED(rc)) {
        fsdevMountDevice("code", g_CodeFileSystem);
        TryMountHblNspOnSd();
    }
    
    return rc;
}

Result ContentManagement::MountCodeForTidSid(Registration::TidSid *tid_sid) {
    return MountCode(tid_sid->title_id, tid_sid->storage_id);
}

Result ContentManagement::ResolveContentPath(char *out_path, u64 tid, FsStorageId sid) {
    Result rc;
    LrRegisteredLocationResolver reg;
    LrLocationResolver lr;
    char path[FS_MAX_PATH] = {0};
    
    /* Try to get the path from the registered resolver. */
    if (R_FAILED(rc = lrOpenRegisteredLocationResolver(&reg))) {
        return rc;
    }
    
    if (R_SUCCEEDED(rc = lrRegLrResolveProgramPath(&reg, tid, path))) {
        strncpy(out_path, path, FS_MAX_PATH);
    } else if (rc != 0x408) {
        return rc;
    }
    
    serviceClose(&reg.s);
    if (R_SUCCEEDED(rc)) {
        return rc;
    }
    
    /* If getting the path from the registered resolver fails, fall back to the normal resolver. */
    if (R_FAILED(rc = lrOpenLocationResolver(sid, &lr))) {
        return rc;
    }
    
    if (R_SUCCEEDED(rc = lrLrResolveProgramPath(&lr, tid, path))) {
        strncpy(out_path, path, FS_MAX_PATH);
    }
    
    serviceClose(&lr.s);
    
    return rc;
}

Result ContentManagement::ResolveContentPathForTidSid(char *out_path, Registration::TidSid *tid_sid) {
    return ResolveContentPath(out_path, tid_sid->title_id, tid_sid->storage_id);
}

Result ContentManagement::RedirectContentPath(const char *path, u64 tid, FsStorageId sid) {
    Result rc;
    LrLocationResolver lr;
    
    if (R_FAILED(rc = lrOpenLocationResolver(sid, &lr))) {
        return rc;
    }
    
    rc = lrLrRedirectProgramPath(&lr, tid, path);
    
    serviceClose(&lr.s);
    
    return rc;
}

Result ContentManagement::RedirectContentPathForTidSid(const char *path, Registration::TidSid *tid_sid) {
    return RedirectContentPath(path, tid_sid->title_id, tid_sid->storage_id);
}

bool ContentManagement::HasCreatedTitle(u64 tid) {
    return std::find(g_created_titles.begin(), g_created_titles.end(), tid) != g_created_titles.end();
}

void ContentManagement::SetCreatedTitle(u64 tid) {
    if (!HasCreatedTitle(tid)) {
        g_created_titles.push_back(tid);
    }
}

static int LoaderIniHandler(void *user, const char *section, const char *name, const char *value) {
    /* Taken and modified, with love, from Rajkosto's implementation. */
    if (strcasecmp(section, "config") == 0) {
        if (strcasecmp(name, "hbl_tid") == 0) {
            u64 override_tid = strtoul(value, NULL, 16);
            if (override_tid != 0) {
                g_override_hbl_tid = override_tid;
            }
        } else if (strcasecmp(name, "hbl_path") == 0) {
            while (*value == '/' || *value == '\\') {
                value++;
            }
            snprintf(g_hbl_sd_path, FS_MAX_PATH, "@Sdcard:/%s", value);
            g_hbl_sd_path[FS_MAX_PATH] = 0;
        } else if (strcasecmp(name, "override_key") == 0) {
            if (value[0] == '!') {
                g_override_by_default = true;
                value++;
            } else {
                g_override_by_default = false;
            }
            
            if (strcasecmp(value, "A") == 0) {
                g_override_key_combination = KEY_A;
            } else if (strcasecmp(value, "B") == 0) {
                g_override_key_combination = KEY_B;
            } else if (strcasecmp(value, "X") == 0) {
                g_override_key_combination = KEY_X;
            } else if (strcasecmp(value, "Y") == 0) {
                g_override_key_combination = KEY_Y;
            } else if (strcasecmp(value, "LS") == 0) {
                g_override_key_combination = KEY_LSTICK;
            } else if (strcasecmp(value, "RS") == 0) {
                g_override_key_combination = KEY_RSTICK;
            } else if (strcasecmp(value, "L") == 0) {
                g_override_key_combination = KEY_L;
            } else if (strcasecmp(value, "R") == 0) {
                g_override_key_combination = KEY_R;
            } else if (strcasecmp(value, "ZL") == 0) {
                g_override_key_combination = KEY_ZL;
            } else if (strcasecmp(value, "ZR") == 0) {
                g_override_key_combination = KEY_ZR;
            } else if (strcasecmp(value, "PLUS") == 0) {
                g_override_key_combination = KEY_PLUS;
            } else if (strcasecmp(value, "MINUS") == 0) {
                g_override_key_combination = KEY_MINUS;
            } else if (strcasecmp(value, "DLEFT") == 0) {
                g_override_key_combination = KEY_DLEFT;
            } else if (strcasecmp(value, "DUP") == 0) {
                g_override_key_combination = KEY_DUP;
            } else if (strcasecmp(value, "DRIGHT") == 0) {
                g_override_key_combination = KEY_DRIGHT;
            } else if (strcasecmp(value, "DDOWN") == 0) {
                g_override_key_combination = KEY_DDOWN;
            } else if (strcasecmp(value, "SL") == 0) {
                g_override_key_combination = KEY_SL;
            } else if (strcasecmp(value, "SR") == 0) {
                g_override_key_combination = KEY_SR;
            } else {
                g_override_key_combination = 0;
            }
        }
    } else {
        return 0;
    }
    return 1;
}

void ContentManagement::LoadConfiguration(FILE *config) {
    if (config == NULL) {
        return;
    }
    
    char *config_str = new char[0x800];
    if (config_str != NULL) {
        /* Read in string. */
        std::fill(config_str, config_str + 0x800, 0);
        fread(config_str, 1, 0x7FF, config);
        
        ini_parse_string(config_str, LoaderIniHandler, NULL);
        
        delete[] config_str;
    }
    
    fclose(config);
}

void ContentManagement::TryMountSdCard() {
    /* Mount SD card, if psc, bus, and pcv have been created. */
    if (!g_has_initialized_fs_dev && HasCreatedTitle(0x0100000000000021) && HasCreatedTitle(0x010000000000000A) && HasCreatedTitle(0x010000000000001A)) {
        Handle tmp_hnd = 0;
        static const char * const required_active_services[] = {"pcv", "gpio", "pinmux", "psc:c"};
        for (unsigned int i = 0; i < sizeof(required_active_services) / sizeof(required_active_services[0]); i++) {
            if (R_FAILED(smGetServiceOriginal(&tmp_hnd, smEncodeName(required_active_services[i])))) {
                return;
            } else {
                svcCloseHandle(tmp_hnd);   
            }
        }
        
        if (R_SUCCEEDED(fsdevMountSdmc())) {
            ContentManagement::LoadConfiguration(fopen("sdmc:/atmosphere/loader.ini", "r"));
            g_has_initialized_fs_dev = true;
        }
    }
}

bool ContentManagement::ShouldReplaceWithHBL(u64 tid) {
    return g_mounted_hbl_nsp && tid == g_override_hbl_tid;
}

bool ContentManagement::ShouldOverrideContents() {
    if (HasCreatedTitle(0x0100000000001000)) {
        u64 kDown = 0;
        bool keys_triggered = (R_SUCCEEDED(HidManagement::GetKeysDown(&kDown)) && ((kDown & g_override_key_combination) != 0));
        return g_has_initialized_fs_dev && (g_override_by_default ^ keys_triggered);
    } else {
        /* Always redirect before qlaunch. */
        return g_has_initialized_fs_dev;
    }
}