#include <switch.h>
#include <stratosphere.hpp>
#include <atomic>

#include "sm_mitm.h"
#include "debug.hpp"
#include "fsmitm_utils.hpp"

static FsFileSystem g_sd_filesystem = {0};
static bool g_has_initialized = false;

static Result EnsureInitialized() {
    if (g_has_initialized) {
        return 0x0;
    }
    
    static const char * const required_active_services[] = {"pcv", "gpio", "pinmux", "psc:c"};
    for (unsigned int i = 0; i < sizeof(required_active_services) / sizeof(required_active_services[0]); i++) {
        Result rc = smMitMUninstall(required_active_services[i]);
        if (rc == 0xE15) {
            return rc;
        } else if (R_FAILED(rc) && rc != 0x1015) {
            return rc;
        }
    }
    
    Result rc = fsMountSdcard(&g_sd_filesystem);
    if (R_SUCCEEDED(rc)) {
        g_has_initialized = true;
    }
    return rc;
}

bool Utils::IsSdInitialized() {
    return R_SUCCEEDED(EnsureInitialized());
}

Result Utils::OpenSdFile(const char *fn, int flags, FsFile *out) {
    Result rc;
    if (R_FAILED((rc = EnsureInitialized()))) {
        return rc;
    }
    
    return fsFsOpenFile(&g_sd_filesystem, fn, flags, out);
}

Result Utils::OpenSdFileForAtmosphere(u64 title_id, const char *fn, int flags, FsFile *out) {
    Result rc;
    if (R_FAILED((rc = EnsureInitialized()))) {
        return rc;
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
    Result rc;
    if (R_FAILED((rc = EnsureInitialized()))) {
        return rc;
    }
    
    return OpenRomFSFile(&g_sd_filesystem, title_id, fn, flags, out);
}

Result Utils::OpenSdDir(const char *path, FsDir *out) {
    Result rc;
    if (R_FAILED((rc = EnsureInitialized()))) {
        return rc;
    }
    
    return fsFsOpenDirectory(&g_sd_filesystem, path, FS_DIROPEN_DIRECTORY | FS_DIROPEN_FILE, out);
}

Result Utils::OpenSdDirForAtmosphere(u64 title_id, const char *path, FsDir *out) {
    Result rc;
    if (R_FAILED((rc = EnsureInitialized()))) {
        return rc;
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
    Result rc;
    if (R_FAILED((rc = EnsureInitialized()))) {
        return rc;
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