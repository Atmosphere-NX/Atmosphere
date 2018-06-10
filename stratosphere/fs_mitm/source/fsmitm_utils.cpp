#include <switch.h>
#include <stratosphere.hpp>
#include <atomic>

#include "fsmitm_utils.hpp"

static FsFileSystem g_sd_filesystem;
static bool g_has_initialized = false;

static Result EnsureInitialized() {
    if (g_has_initialized) {
        return 0x0;
    }
    Result rc = fsMountSdcard(&g_sd_filesystem);
    if (R_SUCCEEDED(rc)) {
        g_has_initialized = true;
    }
    return rc;
}

Result Utils::OpenSdFile(const char *fn, int flags, FsFile *out) {
    Result rc;
    if (R_FAILED((rc = EnsureInitialized()))) {
        return rc;
    }
    char path[FS_MAX_PATH];
    strcpy(path, fn);
    return fsFsOpenFile(&g_sd_filesystem, path, flags, out);
}