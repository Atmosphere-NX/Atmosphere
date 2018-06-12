#include <switch.h>
#include <stratosphere.hpp>
#include <atomic>

#include "sm_mitm.h"

#include "fsmitm_utils.hpp"

static FsFileSystem g_sd_filesystem;
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
        } else if (rc != 0x1015) {
            return rc;
        }
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