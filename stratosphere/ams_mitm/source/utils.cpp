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
#include <stratosphere/spl.hpp>
#include <atomic>
#include <algorithm>
#include <strings.h>

#include "debug.hpp"
#include "utils.hpp"
#include "ini.h"

#include "set_mitm/setsys_settings_items.hpp"
#include "bpc_mitm/bpcmitm_reboot_manager.hpp"

static FsFileSystem g_sd_filesystem = {0};
static HosSignal g_sd_signal, g_hid_signal;

static std::vector<u64> g_mitm_flagged_tids;
static std::vector<u64> g_disable_mitm_flagged_tids;
static std::atomic_bool g_has_initialized = false;
static std::atomic_bool g_has_hid_session = false;

/* Content override support variables/types */
static OverrideKey g_default_override_key = {
    .key_combination = KEY_L,
    .override_by_default = true
};

struct HblOverrideConfig {
    OverrideKey override_key;
    u64 title_id;
    bool override_any_app;
};

static HblOverrideConfig g_hbl_override_config = {
    .override_key = {
        .key_combination = KEY_R,
        .override_by_default = false
    },
    .title_id = static_cast<u64>(sts::ncm::TitleId::AppletPhotoViewer),
    .override_any_app = true
};

/* Static buffer for loader.ini contents at runtime. */
static char g_config_ini_data[0x800];

/* Backup file for CAL0 partition. */
static constexpr size_t ProdinfoSize = 0x8000;
static FsFile g_cal0_file = {};
static FsFile g_bis_key_file = {};
static u8 g_cal0_storage_backup[ProdinfoSize];
static u8 g_cal0_backup[ProdinfoSize];

/* Emummc-related file. */
static FsFile g_emummc_file = {0};

static bool IsHexadecimal(const char *str) {
    while (*str) {
        if (isxdigit((unsigned char)*str)) {
            str++;
        } else {
            return false;
        }
    }
    return true;
}

void Utils::InitializeThreadFunc(void *args) {
    /* Get required services. */
    DoWithSmSession([&]() {
        Handle tmp_hnd = 0;
        static const char * const required_active_services[] = {"pcv", "gpio", "pinmux", "psc:m"};
        for (unsigned int i = 0; i < sts::util::size(required_active_services); i++) {
            R_ASSERT(smGetServiceOriginal(&tmp_hnd, smEncodeName(required_active_services[i])));
            svcCloseHandle(tmp_hnd);
        }
    });

    /* Mount SD. */
    while (R_FAILED(fsMountSdcard(&g_sd_filesystem))) {
        svcSleepThread(1000000ULL);
    }

    /* Back up CAL0, if it's not backed up already. */
    fsFsCreateDirectory(&g_sd_filesystem, "/atmosphere/automatic_backups");
    {
        FsStorage cal0_storage;
        R_ASSERT(fsOpenBisStorage(&cal0_storage, FsBisStorageId_CalibrationBinary));
        R_ASSERT(fsStorageRead(&cal0_storage, 0, g_cal0_storage_backup, ProdinfoSize));
        fsStorageClose(&cal0_storage);

        char serial_number[0x40] = {0};
        memcpy(serial_number, g_cal0_storage_backup + 0x250, 0x18);

        /* Automatically backup PRODINFO. */
        {
            char prodinfo_backup_path[FS_MAX_PATH] = {0};
            if (strlen(serial_number) > 0) {
                snprintf(prodinfo_backup_path, sizeof(prodinfo_backup_path) - 1, "/atmosphere/automatic_backups/%s_PRODINFO.bin", serial_number);
            } else {
                snprintf(prodinfo_backup_path, sizeof(prodinfo_backup_path) - 1, "/atmosphere/automatic_backups/PRODINFO.bin");
            }

            fsFsCreateFile(&g_sd_filesystem, prodinfo_backup_path, ProdinfoSize, 0);
            if (R_SUCCEEDED(fsFsOpenFile(&g_sd_filesystem, prodinfo_backup_path, FS_OPEN_READ | FS_OPEN_WRITE, &g_cal0_file))) {
                bool has_auto_backup = false;
                size_t read = 0;
                if (R_SUCCEEDED(fsFileRead(&g_cal0_file, 0, g_cal0_backup, sizeof(g_cal0_backup), FS_READOPTION_NONE, &read)) && read == sizeof(g_cal0_backup)) {
                    bool is_cal0_valid = true;
                    is_cal0_valid &= memcmp(g_cal0_backup, "CAL0", 4) == 0;
                    is_cal0_valid &= memcmp(g_cal0_backup + 0x250, serial_number, 0x18) == 0;
                    u32 cal0_size = ((u32 *)g_cal0_backup)[2];
                    is_cal0_valid &= cal0_size + 0x40 <= ProdinfoSize;
                    if (is_cal0_valid) {
                        u8 calc_hash[0x20];
                        sha256CalculateHash(calc_hash, g_cal0_backup + 0x40, cal0_size);
                        is_cal0_valid &= memcmp(calc_hash, g_cal0_backup + 0x20, sizeof(calc_hash)) == 0;
                    }
                    has_auto_backup = is_cal0_valid;
                }

                if (!has_auto_backup) {
                    fsFileSetSize(&g_cal0_file, ProdinfoSize);
                    fsFileWrite(&g_cal0_file, 0, g_cal0_storage_backup, ProdinfoSize, FS_WRITEOPTION_FLUSH);
                }

                /* NOTE: g_cal0_file is intentionally not closed here. This prevents any other process from opening it. */
                memset(g_cal0_storage_backup, 0, sizeof(g_cal0_storage_backup));
                memset(g_cal0_backup, 0, sizeof(g_cal0_backup));
            }
        }

        /* Automatically backup BIS keys. */
        {
            u64 key_generation = 0;
            if (GetRuntimeFirmwareVersion() >= FirmwareVersion_500) {
                R_ASSERT(splGetConfig(SplConfigItem_NewKeyGeneration, &key_generation));
            }

            static constexpr u8 const BisKeySources[4][2][0x10] = {
                {
                    {0xF8, 0x3F, 0x38, 0x6E, 0x2C, 0xD2, 0xCA, 0x32, 0xA8, 0x9A, 0xB9, 0xAA, 0x29, 0xBF, 0xC7, 0x48},
                    {0x7D, 0x92, 0xB0, 0x3A, 0xA8, 0xBF, 0xDE, 0xE1, 0xA7, 0x4C, 0x3B, 0x6E, 0x35, 0xCB, 0x71, 0x06}
                },
                {
                    {0x41, 0x00, 0x30, 0x49, 0xDD, 0xCC, 0xC0, 0x65, 0x64, 0x7A, 0x7E, 0xB4, 0x1E, 0xED, 0x9C, 0x5F},
                    {0x44, 0x42, 0x4E, 0xDA, 0xB4, 0x9D, 0xFC, 0xD9, 0x87, 0x77, 0x24, 0x9A, 0xDC, 0x9F, 0x7C, 0xA4}
                },
                {
                    {0x52, 0xC2, 0xE9, 0xEB, 0x09, 0xE3, 0xEE, 0x29, 0x32, 0xA1, 0x0C, 0x1F, 0xB6, 0xA0, 0x92, 0x6C},
                    {0x4D, 0x12, 0xE1, 0x4B, 0x2A, 0x47, 0x4C, 0x1C, 0x09, 0xCB, 0x03, 0x59, 0xF0, 0x15, 0xF4, 0xE4}
                },
                {
                    {0x52, 0xC2, 0xE9, 0xEB, 0x09, 0xE3, 0xEE, 0x29, 0x32, 0xA1, 0x0C, 0x1F, 0xB6, 0xA0, 0x92, 0x6C},
                    {0x4D, 0x12, 0xE1, 0x4B, 0x2A, 0x47, 0x4C, 0x1C, 0x09, 0xCB, 0x03, 0x59, 0xF0, 0x15, 0xF4, 0xE4}
                }
            };
            u8 bis_keys[4][2][0x10] = {};

            /* TODO: Clean this up in ams_mitm refactor. */
            for (size_t partition = 0; partition < 4; partition++) {
                if (partition == 0) {
                    for (size_t i = 0; i < 2; i++) {
                        R_ASSERT(splFsGenerateSpecificAesKey(BisKeySources[partition][i], key_generation, i, bis_keys[partition][i]));
                    }
                } else {
                    static constexpr u8 const BisKekSource[0x10] = {
                        0x34, 0xC1, 0xA0, 0xC4, 0x82, 0x58, 0xF8, 0xB4, 0xFA, 0x9E, 0x5E, 0x6A, 0xDA, 0xFC, 0x7E, 0x4F,
                    };

                    const u32 option = (partition == 3 && sts::spl::IsRecoveryBoot()) ? 0x4 : 0x1;

                    u8 access_key[0x10];
                    R_ASSERT(splCryptoGenerateAesKek(BisKekSource, key_generation, option, access_key));
                    for (size_t i = 0; i < 2; i++) {
                        R_ASSERT(splCryptoGenerateAesKey(access_key, BisKeySources[partition][i], bis_keys[partition][i]));
                    }
                }
            }

            char bis_key_backup_path[FS_MAX_PATH] = {0};
            if (strlen(serial_number) > 0) {
                snprintf(bis_key_backup_path, sizeof(bis_key_backup_path) - 1, "/atmosphere/automatic_backups/%s_BISKEYS.bin", serial_number);
            } else {
                snprintf(bis_key_backup_path, sizeof(bis_key_backup_path) - 1, "/atmosphere/automatic_backups/BISKEYS.bin");
            }

            fsFsCreateFile(&g_sd_filesystem, bis_key_backup_path, sizeof(bis_keys), 0);
            if (R_SUCCEEDED(fsFsOpenFile(&g_sd_filesystem, bis_key_backup_path, FS_OPEN_READ | FS_OPEN_WRITE, &g_bis_key_file))) {
                fsFileSetSize(&g_bis_key_file, sizeof(bis_keys));
                fsFileWrite(&g_bis_key_file, 0, bis_keys, sizeof(bis_keys), FS_WRITEOPTION_FLUSH);
                /* NOTE: g_bis_key_file is intentionally not closed here. This prevents any other process from opening it. */
            }
        }
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
                strcat(title_path, "/flags/fsmitm.flag");
                if (R_SUCCEEDED(fsFsOpenFile(&g_sd_filesystem, title_path, FS_OPEN_READ, &f))) {
                    g_mitm_flagged_tids.push_back(title_id);
                    fsFileClose(&f);
                } else {
                    /* TODO: Deprecate. */
                    memset(title_path, 0, sizeof(title_path));
                    strcpy(title_path, "/atmosphere/titles/");
                    strcat(title_path, dir_entry.name);
                    strcat(title_path, "/fsmitm.flag");
                    if (R_SUCCEEDED(fsFsOpenFile(&g_sd_filesystem, title_path, FS_OPEN_READ, &f))) {
                        g_mitm_flagged_tids.push_back(title_id);
                        fsFileClose(&f);
                    }
                }

                memset(title_path, 0, sizeof(title_path));
                strcpy(title_path, "/atmosphere/titles/");
                strcat(title_path, dir_entry.name);
                strcat(title_path, "/flags/fsmitm_disable.flag");
                if (R_SUCCEEDED(fsFsOpenFile(&g_sd_filesystem, title_path, FS_OPEN_READ, &f))) {
                    g_disable_mitm_flagged_tids.push_back(title_id);
                    fsFileClose(&f);
                } else {
                    /* TODO: Deprecate. */
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
        }
        fsDirClose(&titles_dir);
    }

    Utils::RefreshConfiguration();

    /* If we're emummc, persist a write handle to prevent other processes from touching the image. */
    if (IsEmummc()) {
        const char *emummc_file_path = GetEmummcFilePath();
        if (emummc_file_path != nullptr) {
            char emummc_path[0x100] = {0};
            std::strncpy(emummc_path, emummc_file_path, 0x80);
            std::strcat(emummc_path, "/eMMC");
            fsFsOpenFile(&g_sd_filesystem, emummc_file_path, FS_OPEN_READ | FS_OPEN_WRITE, &g_emummc_file);
        }
    }

    /* Initialize set:sys. */
    DoWithSmSession([&]() {
        R_ASSERT(setsysInitialize());
    });

    /* Signal SD is initialized. */
    g_has_initialized = true;

    /* Load custom settings configuration. */
    SettingsItemManager::LoadConfiguration();

    /* Signal to waiters that we are ready. */
    g_sd_signal.Signal();

    /* Initialize HID. */
    while (!g_has_hid_session) {
        DoWithSmSession([&]() {
            if (R_SUCCEEDED(hidInitialize())) {
                g_has_hid_session = true;
            }
        });
        if (!g_has_hid_session) {
            svcSleepThread(1000000ULL);
        }
    }

    /* Signal to waiters that we are ready. */
    g_hid_signal.Signal();
}

bool Utils::IsSdInitialized() {
    return g_has_initialized;
}

void Utils::WaitSdInitialized() {
    g_sd_signal.Wait();
}

bool Utils::IsHidAvailable() {
    return g_has_hid_session;
}

void Utils::WaitHidAvailable() {
    g_hid_signal.Wait();
}

Result Utils::OpenSdFile(const char *fn, int flags, FsFile *out) {
    if (!IsSdInitialized()) {
        return ResultFsSdCardNotPresent;
    }

    return fsFsOpenFile(&g_sd_filesystem, fn, flags, out);
}

Result Utils::OpenSdFileForAtmosphere(u64 title_id, const char *fn, int flags, FsFile *out) {
    if (!IsSdInitialized()) {
        return ResultFsSdCardNotPresent;
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
        return ResultFsSdCardNotPresent;
    }

    return OpenRomFSFile(&g_sd_filesystem, title_id, fn, flags, out);
}

Result Utils::OpenSdDir(const char *path, FsDir *out) {
    if (!IsSdInitialized()) {
        return ResultFsSdCardNotPresent;
    }

    return fsFsOpenDirectory(&g_sd_filesystem, path, FS_DIROPEN_DIRECTORY | FS_DIROPEN_FILE, out);
}

Result Utils::OpenSdDirForAtmosphere(u64 title_id, const char *path, FsDir *out) {
    if (!IsSdInitialized()) {
        return ResultFsSdCardNotPresent;
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
        return ResultFsSdCardNotPresent;
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

bool Utils::HasSdRomfsContent(u64 title_id) {
    /* Check for romfs.bin. */
    FsFile data_file;
    if (R_SUCCEEDED(Utils::OpenSdFileForAtmosphere(title_id, "romfs.bin", FS_OPEN_READ, &data_file))) {
        fsFileClose(&data_file);
        return true;
    }

    /* Check for romfs folder with non-zero content. */
    FsDir dir;
    if (R_FAILED(Utils::OpenRomFSSdDir(title_id, "", &dir))) {
        return false;
    }
    ON_SCOPE_EXIT {
        fsDirClose(&dir);
    };

    FsDirectoryEntry dir_entry;
    u64 read_entries;
    return R_SUCCEEDED(fsDirRead(&dir, 0, &read_entries, 1, &dir_entry)) && read_entries == 1;
}

Result Utils::SaveSdFileForAtmosphere(u64 title_id, const char *fn, void *data, size_t size) {
    if (!IsSdInitialized()) {
        return ResultFsSdCardNotPresent;
    }

    char path[FS_MAX_PATH];
    if (*fn == '/') {
        snprintf(path, sizeof(path), "/atmosphere/titles/%016lx%s", title_id, fn);
    } else {
        snprintf(path, sizeof(path), "/atmosphere/titles/%016lx/%s", title_id, fn);
    }

    /* Unconditionally create. */
    FsFile f;
    fsFsCreateFile(&g_sd_filesystem, path, size, 0);

    /* Try to open. */
    R_TRY(fsFsOpenFile(&g_sd_filesystem, path, FS_OPEN_READ | FS_OPEN_WRITE, &f));
    ON_SCOPE_EXIT { fsFileClose(&f); };

    /* Try to make it big enough. */
    R_TRY(fsFileSetSize(&f, size));

    /* Try to write the data. */
    R_TRY(fsFileWrite(&f, 0, data, size, FS_WRITEOPTION_FLUSH));

    return ResultSuccess;
}

bool Utils::IsHblTid(u64 _tid) {
    const sts::ncm::TitleId tid{_tid};
    return (g_hbl_override_config.override_any_app && sts::ncm::IsApplicationTitleId(tid)) || (_tid == g_hbl_override_config.title_id);
}

bool Utils::IsWebAppletTid(u64 _tid) {
    const sts::ncm::TitleId tid{_tid};
    return tid == sts::ncm::TitleId::AppletWeb || tid == sts::ncm::TitleId::AppletOfflineWeb || tid == sts::ncm::TitleId::AppletLoginShare || tid == sts::ncm::TitleId::AppletWifiWebAuth;
}

bool Utils::HasTitleFlag(u64 tid, const char *flag) {
    if (IsSdInitialized()) {
        FsFile f;
        char flag_path[FS_MAX_PATH];

        memset(flag_path, 0, sizeof(flag_path));
        snprintf(flag_path, sizeof(flag_path) - 1, "flags/%s.flag", flag);
        if (R_SUCCEEDED(OpenSdFileForAtmosphere(tid, flag_path, FS_OPEN_READ, &f))) {
            fsFileClose(&f);
            return true;
        }

        /* TODO: Deprecate. */
        snprintf(flag_path, sizeof(flag_path) - 1, "%s.flag", flag);
        if (R_SUCCEEDED(OpenSdFileForAtmosphere(tid, flag_path, FS_OPEN_READ, &f))) {
            fsFileClose(&f);
            return true;
        }
    }
    return false;
}

bool Utils::HasGlobalFlag(const char *flag) {
    if (IsSdInitialized()) {
        FsFile f;
        char flag_path[FS_MAX_PATH] = {0};
        snprintf(flag_path, sizeof(flag_path), "/atmosphere/flags/%s.flag", flag);
        if (R_SUCCEEDED(fsFsOpenFile(&g_sd_filesystem, flag_path, FS_OPEN_READ, &f))) {
            fsFileClose(&f);
            return true;
        }
    }
    return false;
}

bool Utils::HasHblFlag(const char *flag) {
    char hbl_flag[FS_MAX_PATH] = {0};
    snprintf(hbl_flag, sizeof(hbl_flag), "hbl_%s", flag);
    return HasGlobalFlag(hbl_flag);
}

bool Utils::HasFlag(u64 tid, const char *flag) {
    return HasTitleFlag(tid, flag) || (IsHblTid(tid) && HasHblFlag(flag));
}

bool Utils::HasSdMitMFlag(u64 tid) {
    if (IsHblTid(tid)) {
        return true;
    }

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

Result Utils::GetKeysHeld(u64 *keys) {
    if (!Utils::IsHidAvailable()) {
        return MAKERESULT(Module_Libnx, LibnxError_InitFail_HID);
    }

    hidScanInput();
    *keys = 0;

    for (int controller = 0; controller < 10; controller++) {
        *keys |= hidKeysHeld((HidControllerID) controller);
    }

    return ResultSuccess;
}

static bool HasOverrideKey(OverrideKey *cfg) {
    u64 kDown = 0;
    bool keys_triggered = (R_SUCCEEDED(Utils::GetKeysHeld(&kDown)) && ((kDown & cfg->key_combination) != 0));
    return Utils::IsSdInitialized() && (cfg->override_by_default ^ keys_triggered);
}


bool Utils::HasOverrideButton(u64 tid) {
    if ((!sts::ncm::IsApplicationTitleId(sts::ncm::TitleId{tid})) || (!IsSdInitialized())) {
        /* Disable button override disable for non-applications. */
        return true;
    }

    /* Unconditionally refresh loader.ini contents. */
    RefreshConfiguration();

    if (IsHblTid(tid) && HasOverrideKey(&g_hbl_override_config.override_key)) {
        return true;
    }

    OverrideKey title_cfg = GetTitleOverrideKey(tid);
    return HasOverrideKey(&title_cfg);
}

static OverrideKey ParseOverrideKey(const char *value) {
    OverrideKey cfg;

    /* Parse on by default. */
    if (value[0] == '!') {
        cfg.override_by_default = true;
        value++;
    } else {
        cfg.override_by_default = false;
    }

    /* Parse key combination. */
    if (strcasecmp(value, "A") == 0) {
        cfg.key_combination = KEY_A;
    } else if (strcasecmp(value, "B") == 0) {
        cfg.key_combination = KEY_B;
    } else if (strcasecmp(value, "X") == 0) {
        cfg.key_combination = KEY_X;
    } else if (strcasecmp(value, "Y") == 0) {
        cfg.key_combination = KEY_Y;
    } else if (strcasecmp(value, "LS") == 0) {
        cfg.key_combination = KEY_LSTICK;
    } else if (strcasecmp(value, "RS") == 0) {
        cfg.key_combination = KEY_RSTICK;
    } else if (strcasecmp(value, "L") == 0) {
        cfg.key_combination = KEY_L;
    } else if (strcasecmp(value, "R") == 0) {
        cfg.key_combination = KEY_R;
    } else if (strcasecmp(value, "ZL") == 0) {
        cfg.key_combination = KEY_ZL;
    } else if (strcasecmp(value, "ZR") == 0) {
        cfg.key_combination = KEY_ZR;
    } else if (strcasecmp(value, "PLUS") == 0) {
        cfg.key_combination = KEY_PLUS;
    } else if (strcasecmp(value, "MINUS") == 0) {
        cfg.key_combination = KEY_MINUS;
    } else if (strcasecmp(value, "DLEFT") == 0) {
        cfg.key_combination = KEY_DLEFT;
    } else if (strcasecmp(value, "DUP") == 0) {
        cfg.key_combination = KEY_DUP;
    } else if (strcasecmp(value, "DRIGHT") == 0) {
        cfg.key_combination = KEY_DRIGHT;
    } else if (strcasecmp(value, "DDOWN") == 0) {
        cfg.key_combination = KEY_DDOWN;
    } else if (strcasecmp(value, "SL") == 0) {
        cfg.key_combination = KEY_SL;
    } else if (strcasecmp(value, "SR") == 0) {
        cfg.key_combination = KEY_SR;
    } else {
        cfg.key_combination = 0;
    }

    return cfg;
}

static int FsMitmIniHandler(void *user, const char *section, const char *name, const char *value) {
    /* Taken and modified, with love, from Rajkosto's implementation. */
    if (strcasecmp(section, "hbl_config") == 0) {
        if (strcasecmp(name, "title_id") == 0) {
            if (strcasecmp(value, "app") == 0) {
                /* DEPRECATED */
                g_hbl_override_config.override_any_app = true;
                g_hbl_override_config.title_id = 0;
            } else {
                u64 override_tid = strtoul(value, NULL, 16);
                if (override_tid != 0) {
                    g_hbl_override_config.title_id = override_tid;
                }
            }
        } else if (strcasecmp(name, "override_key") == 0) {
            g_hbl_override_config.override_key = ParseOverrideKey(value);
        } else if (strcasecmp(name, "override_any_app") == 0) {
            if (strcasecmp(value, "true") == 0 || strcasecmp(value, "1") == 0) {
                g_hbl_override_config.override_any_app = true;
            } else if (strcasecmp(value, "false") == 0 || strcasecmp(value, "0") == 0) {
                g_hbl_override_config.override_any_app = false;
            } else {
                /* I guess we default to not changing the value? */
            }
        }
    } else if (strcasecmp(section, "default_config") == 0) {
        if (strcasecmp(name, "override_key") == 0) {
            g_default_override_key = ParseOverrideKey(value);
        }
    } else {
        return 0;
    }
    return 1;
}

static int FsMitmTitleSpecificIniHandler(void *user, const char *section, const char *name, const char *value) {
    /* We'll output an override key when relevant. */
    OverrideKey *user_cfg = reinterpret_cast<OverrideKey *>(user);

    if (strcasecmp(section, "override_config") == 0) {
        if (strcasecmp(name, "override_key") == 0) {
            *user_cfg = ParseOverrideKey(value);
        }
    } else {
        return 0;
    }
    return 1;
}

OverrideKey Utils::GetTitleOverrideKey(u64 tid) {
    OverrideKey cfg = g_default_override_key;
    char path[FS_MAX_PATH+1] = {0};
    snprintf(path, FS_MAX_PATH, "/atmosphere/titles/%016lx/config.ini", tid);
    FsFile cfg_file;

    if (R_SUCCEEDED(fsFsOpenFile(&g_sd_filesystem, path, FS_OPEN_READ, &cfg_file))) {
        ON_SCOPE_EXIT { fsFileClose(&cfg_file); };

        size_t config_file_size = 0x20000;
        fsFileGetSize(&cfg_file, &config_file_size);

        char *config_buf = reinterpret_cast<char *>(calloc(1, config_file_size + 1));
        if (config_buf != NULL) {
            ON_SCOPE_EXIT { free(config_buf); };

            /* Read title ini contents. */
            fsFileRead(&cfg_file, 0, config_buf, config_file_size, FS_READOPTION_NONE, &config_file_size);

            /* Parse title ini. */
            ini_parse_string(config_buf, FsMitmTitleSpecificIniHandler, &cfg);
        }
    }

    return cfg;
}

static int FsMitmTitleSpecificLocaleIniHandler(void *user, const char *section, const char *name, const char *value) {
    /* We'll output an override locale when relevant. */
    OverrideLocale *user_locale = reinterpret_cast<OverrideLocale *>(user);

    if (strcasecmp(section, "override_config") == 0) {
        if (strcasecmp(name, "override_language") == 0) {
            user_locale->language_code = EncodeLanguageCode(value);
        } else if (strcasecmp(name, "override_region") == 0) {
            if (strcasecmp(value, "jpn") == 0) {
                user_locale->region_code = RegionCode_Japan;
            } else if (strcasecmp(value, "usa") == 0) {
                user_locale->region_code = RegionCode_America;
            } else if (strcasecmp(value, "eur") == 0) {
                user_locale->region_code = RegionCode_Europe;
            } else if (strcasecmp(value, "aus") == 0) {
                user_locale->region_code = RegionCode_Australia;
            } else if (strcasecmp(value, "chn") == 0) {
                user_locale->region_code = RegionCode_China;
            } else if (strcasecmp(value, "kor") == 0) {
                user_locale->region_code = RegionCode_Korea;
            } else if (strcasecmp(value, "twn") == 0) {
                user_locale->region_code = RegionCode_Taiwan;
            }
        }
    } else {
        return 0;
    }
    return 1;
}

OverrideLocale Utils::GetTitleOverrideLocale(u64 tid) {
    OverrideLocale locale;
    std::memset(&locale, 0xCC, sizeof(locale));
    char path[FS_MAX_PATH+1] = {0};
    snprintf(path, FS_MAX_PATH, "/atmosphere/titles/%016lx/config.ini", tid);
    FsFile cfg_file;

    if (R_SUCCEEDED(fsFsOpenFile(&g_sd_filesystem, path, FS_OPEN_READ, &cfg_file))) {
        ON_SCOPE_EXIT { fsFileClose(&cfg_file); };

        size_t config_file_size = 0x20000;
        fsFileGetSize(&cfg_file, &config_file_size);

        char *config_buf = reinterpret_cast<char *>(calloc(1, config_file_size + 1));
        if (config_buf != NULL) {
            ON_SCOPE_EXIT { free(config_buf); };

            /* Read title ini contents. */
            fsFileRead(&cfg_file, 0, config_buf, config_file_size, FS_READOPTION_NONE, &config_file_size);

            /* Parse title ini. */
            ini_parse_string(config_buf, FsMitmTitleSpecificLocaleIniHandler, &locale);
        }
    }

    return locale;
}

void Utils::RefreshConfiguration() {
    FsFile config_file;
    if (R_FAILED(fsFsOpenFile(&g_sd_filesystem, "/atmosphere/loader.ini", FS_OPEN_READ, &config_file))) {
        return;
    }

    u64 size;
    if (R_FAILED(fsFileGetSize(&config_file, &size))) {
        return;
    }

    size = std::min(size, (decltype(size))0x7FF);

    /* Read in string. */
    std::fill(g_config_ini_data, g_config_ini_data + 0x800, 0);
    size_t r_s;
    fsFileRead(&config_file, 0, g_config_ini_data, size, FS_READOPTION_NONE, &r_s);
    fsFileClose(&config_file);

    ini_parse_string(g_config_ini_data, FsMitmIniHandler, NULL);
}

Result Utils::GetSettingsItemValueSize(const char *name, const char *key, u64 *out_size) {
    return SettingsItemManager::GetValueSize(name, key, out_size);
}

Result Utils::GetSettingsItemValue(const char *name, const char *key, void *out, size_t max_size, u64 *out_size) {
    return SettingsItemManager::GetValue(name, key, out, max_size, out_size);
}

Result Utils::GetSettingsItemBooleanValue(const char *name, const char *key, bool *out) {
    u8 val = 0;
    u64 out_size;
    R_TRY(Utils::GetSettingsItemValue(name, key, &val, sizeof(val), &out_size));

    if (out) {
        *out = val != 0;
    }
    return ResultSuccess;
}

void Utils::RebootToFatalError(AtmosphereFatalErrorContext *ctx) {
    BpcRebootManager::RebootForFatalError(ctx);
}
