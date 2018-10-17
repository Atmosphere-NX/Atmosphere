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
#include <atomic>
#include <algorithm>
#include <strings.h>

#include "sm_mitm.h"
#include "debug.hpp"
#include "fsmitm_utils.hpp"
#include "ini.h"

static FsFileSystem g_sd_filesystem = {0};
static std::vector<u64> g_mitm_flagged_tids;
static std::vector<u64> g_disable_mitm_flagged_tids;
static std::atomic_bool g_has_initialized = false;
static std::atomic_bool g_has_hid_session = false;

static u64 g_override_key_combination = KEY_R;
static bool g_override_by_default = true;

static bool IsHexadecimal(const char *str) {
    while (*str) {
        if (isxdigit(*str)) {
            str++;
        } else {
            return false;
        }
    }
    return true;
}

void Utils::InitializeSdThreadFunc(void *args) {
    /* Get required services. */
    Handle tmp_hnd = 0;
    static const char * const required_active_services[] = {"pcv", "gpio", "pinmux", "psc:c"};
    for (unsigned int i = 0; i < sizeof(required_active_services) / sizeof(required_active_services[0]); i++) {
        if (R_FAILED(smGetServiceOriginal(&tmp_hnd, smEncodeName(required_active_services[i])))) {
            /* TODO: Panic */
        } else {
            svcCloseHandle(tmp_hnd);   
        }
    }
    
    /* Mount SD. */
    while (R_FAILED(fsMountSdcard(&g_sd_filesystem))) {
        svcSleepThread(1000ULL);
    }
    
    /* Check for MitM flags. */
    FsDir titles_dir;
    if (R_SUCCEEDED(fsFsOpenDirectory(&g_sd_filesystem, "/atmosphere/titles", FS_DIROPEN_DIRECTORY, &titles_dir))) {
        FsDirectoryEntry dir_entry;
        FsFile f;
        u64 read_entries;
        while (R_SUCCEEDED((fsDirRead(&titles_dir, 0, &read_entries, 1, &dir_entry))) && read_entries == 1) {
            if (strlen(dir_entry.name) == 0x10 && IsHexadecimal(dir_entry.name)) {
                u64 title_id = strtoul(dir_entry.name, NULL, 16);
                char title_path[FS_MAX_PATH] = {0};
                strcpy(title_path, "/atmosphere/titles/");
                strcat(title_path, dir_entry.name);
                strcat(title_path, "/fsmitm.flag");
                if (R_SUCCEEDED(fsFsOpenFile(&g_sd_filesystem, title_path, FS_OPEN_READ, &f))) {
                    g_mitm_flagged_tids.push_back(title_id);
                    fsFileClose(&f);
                }
                
                memset(title_path, 0, sizeof(title_path));
                strcpy(title_path, "/atmosphere/titles/");
                strcat(title_path, dir_entry.name);
                strcat(title_path, "/fsmitm_disable.flag");
                if (R_SUCCEEDED(fsFsOpenFile(&g_sd_filesystem, title_path, FS_OPEN_READ, &f))) {
                    g_disable_mitm_flagged_tids.push_back(title_id);
                    fsFileClose(&f);
                }
            }
        }
        fsDirClose(&titles_dir);
    }
    
    FsFile config_file;
    if (R_SUCCEEDED(fsFsOpenFile(&g_sd_filesystem, "/atmosphere/loader.ini", FS_OPEN_READ, &config_file))) {
        Utils::LoadConfiguration(&config_file);
        fsFileClose(&config_file);
    }

    
    g_has_initialized = true;
    
    svcExitThread();
}

void Utils::InitializeHidThreadFunc(void *args) {
    while (R_FAILED(hidInitialize())) {
        svcSleepThread(1000ULL);
    }
    
    g_has_hid_session = true;
    
    svcExitThread();
}

bool Utils::IsSdInitialized() {
    return g_has_initialized;
}

bool Utils::IsHidInitialized() {
    return g_has_hid_session;
}

Result Utils::OpenSdFile(const char *fn, int flags, FsFile *out) {
    if (!IsSdInitialized()) {
        return 0xFA202;
    }
    
    return fsFsOpenFile(&g_sd_filesystem, fn, flags, out);
}

Result Utils::OpenSdFileForAtmosphere(u64 title_id, const char *fn, int flags, FsFile *out) {
    if (!IsSdInitialized()) {
        return 0xFA202;
    }
    
    char path[FS_MAX_PATH];
    if (*fn == '/') {
        snprintf(path, sizeof(path), "/atmosphere/titles/%016lx%s", title_id, fn);
    } else {
        snprintf(path, sizeof(path), "/atmosphere/titles/%016lx/%s", title_id, fn);
    }
    return fsFsOpenFile(&g_sd_filesystem, path, flags, out);
}

Result Utils::OpenRomFSSdFile(u64 title_id, const char *fn, int flags, FsFile *out) {
    if (!IsSdInitialized()) {
        return 0xFA202;
    }
    
    return OpenRomFSFile(&g_sd_filesystem, title_id, fn, flags, out);
}

Result Utils::OpenSdDir(const char *path, FsDir *out) {
    if (!IsSdInitialized()) {
        return 0xFA202;
    }
    
    return fsFsOpenDirectory(&g_sd_filesystem, path, FS_DIROPEN_DIRECTORY | FS_DIROPEN_FILE, out);
}

Result Utils::OpenSdDirForAtmosphere(u64 title_id, const char *path, FsDir *out) {
    if (!IsSdInitialized()) {
        return 0xFA202;
    }
    
    char safe_path[FS_MAX_PATH];
    if (*path == '/') {
        snprintf(safe_path, sizeof(safe_path), "/atmosphere/titles/%016lx%s", title_id, path);
    } else {
        snprintf(safe_path, sizeof(safe_path), "/atmosphere/titles/%016lx/%s", title_id, path);
    }
    return fsFsOpenDirectory(&g_sd_filesystem, safe_path, FS_DIROPEN_DIRECTORY | FS_DIROPEN_FILE, out);
}

Result Utils::OpenRomFSSdDir(u64 title_id, const char *path, FsDir *out) {
    if (!IsSdInitialized()) {
        return 0xFA202;
    }
    
    return OpenRomFSDir(&g_sd_filesystem, title_id, path, out);
}


Result Utils::OpenRomFSFile(FsFileSystem *fs, u64 title_id, const char *fn, int flags, FsFile *out) {
    char path[FS_MAX_PATH];
    if (*fn == '/') {
        snprintf(path, sizeof(path), "/atmosphere/titles/%016lx/romfs%s", title_id, fn);
    } else {
        snprintf(path, sizeof(path), "/atmosphere/titles/%016lx/romfs/%s", title_id, fn);
    }
    return fsFsOpenFile(fs, path, flags, out);
}

Result Utils::OpenRomFSDir(FsFileSystem *fs, u64 title_id, const char *path, FsDir *out) {
    char safe_path[FS_MAX_PATH];
    if (*path == '/') {
        snprintf(safe_path, sizeof(safe_path), "/atmosphere/titles/%016lx/romfs%s", title_id, path);
    } else {
        snprintf(safe_path, sizeof(safe_path), "/atmosphere/titles/%016lx/romfs/%s", title_id, path);
    }
    return fsFsOpenDirectory(fs, safe_path, FS_DIROPEN_DIRECTORY | FS_DIROPEN_FILE, out);
}

Result Utils::HasSdRomfsContent(u64 title_id, bool *out) {
    Result rc;
    FsDir dir;
    if (R_FAILED((rc = Utils::OpenRomFSSdDir(title_id, "", &dir)))) {
        return rc;
    }
    
    FsDirectoryEntry dir_entry;
    u64 read_entries;
    if (R_SUCCEEDED((rc = fsDirRead(&dir, 0, &read_entries, 1, &dir_entry)))) {
        *out = (read_entries == 1);
    }
    fsDirClose(&dir);
    return rc;
}

bool Utils::HasSdMitMFlag(u64 tid) {
    if (IsSdInitialized()) {
        return std::find(g_mitm_flagged_tids.begin(), g_mitm_flagged_tids.end(), tid) != g_mitm_flagged_tids.end();
    }
    return false;
}

bool Utils::HasSdDisableMitMFlag(u64 tid) {
    if (IsSdInitialized()) {
        return std::find(g_disable_mitm_flagged_tids.begin(), g_disable_mitm_flagged_tids.end(), tid) != g_disable_mitm_flagged_tids.end();
    }
    return false;
}

Result Utils::GetKeysDown(u64 *keys) {
    if (!Utils::IsHidInitialized()) {
        return MAKERESULT(Module_Libnx, LibnxError_InitFail_HID);
    }
    
    hidScanInput();
    *keys = hidKeysDown(CONTROLLER_P1_AUTO);
    
    return 0x0;
}

bool Utils::HasOverrideButton(u64 tid) {
    if (tid < 0x0100000000010000ULL) {
        /* Disable button override disable for non-applications. */
        return true;
    }
    
    u64 kDown = 0;
    bool keys_triggered = (R_SUCCEEDED(GetKeysDown(&kDown)) && ((kDown & g_override_key_combination) != 0));
    return IsSdInitialized() && (g_override_by_default ^ keys_triggered);
}

static int FsMitMIniHandler(void *user, const char *section, const char *name, const char *value) {
    /* Taken and modified, with love, from Rajkosto's implementation. */
    if (strcasecmp(section, "config") == 0) {
        if (strcasecmp(name, "override_key") == 0) {
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


void Utils::LoadConfiguration(FsFile *f) {
    if (f == NULL) {
        return;
    }
    
    u64 size;
    if (R_FAILED(fsFileGetSize(f, &size))) {
        return;
    }
    
    size = std::min(size, (decltype(size))0x7FF);
    
    char *config_str = new char[0x800];
    if (config_str != NULL) {
        /* Read in string. */
        std::fill(config_str, config_str + 0x800, 0);
        size_t r_s;
        fsFileRead(f, 0, config_str, size, &r_s);
        
        ini_parse_string(config_str, FsMitMIniHandler, NULL);
        
        delete[] config_str;
    }
}