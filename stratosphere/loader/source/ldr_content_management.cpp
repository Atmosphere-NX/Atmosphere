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
#include <dirent.h>
#include <stratosphere/cfg.hpp>

#include "ldr_content_management.hpp"
#include "ldr_ecs.hpp"

namespace sts::ldr {

    namespace {

        /* DeviceNames. */
        constexpr const char *CodeFileSystemDeviceName   = "code";
        constexpr const char *HblFileSystemDeviceName    = "hbl";
        constexpr const char *SdCardFileSystemDeviceName = "sdmc";

        constexpr const char *SdCardStorageMountPoint = "@Sdcard";

        /* Globals. */
        bool g_has_mounted_sd_card = false;

        ncm::TitleId g_should_override_title_id;
        bool g_should_override_hbl = false;
        bool g_should_override_sd  = false;

        /* Helpers. */
        inline void FixFileSystemPath(char *path) {
            /* Paths will fail when passed to FS if they use the wrong kinds of slashes. */
            for (size_t i = 0; i < FS_MAX_PATH && path[i]; i++) {
                if (path[i] == '\\') {
                    path[i] = '/';
                }
            }
        }

        inline const char *GetRelativePathStart(const char *relative_path) {
            /* We assume filenames don't start with slashes when formatting. */
            while (*relative_path == '/' || *relative_path == '\\') {
                relative_path++;
            }
            return relative_path;
        }

        void UpdateShouldOverrideCache(ncm::TitleId title_id) {
            if (g_should_override_title_id != title_id) {
                cfg::GetOverrideKeyHeldStatus(&g_should_override_hbl, &g_should_override_sd, title_id);
            }
            g_should_override_title_id = title_id;
        }

        void InvalidateShouldOverrideCache() {
            g_should_override_title_id = {};
        }

        bool ShouldOverrideWithHbl(ncm::TitleId title_id) {
            UpdateShouldOverrideCache(title_id);
            return g_should_override_hbl;
        }

        bool ShouldOverrideWithSd(ncm::TitleId title_id) {
            UpdateShouldOverrideCache(title_id);
            return g_should_override_sd;
        }

        Result MountSdCardFileSystem() {
            return fsdevMountSdmc();
        }

        Result MountNspFileSystem(const char *device_name, const char *path) {
            FsFileSystem fs;
            R_TRY(fsOpenFileSystemWithId(&fs, 0, FsFileSystemType_ApplicationPackage, path));
            if(fsdevMountDevice(device_name, fs) < 0) {
                std::abort();
            }
            return ResultSuccess;
        }

        FILE *OpenFile(const char *device_name, const char *relative_path) {
            /* Allow nullptr device_name/relative path -- those are simply not openable. */
            if (device_name == nullptr || relative_path == nullptr) {
                return nullptr;
            }

            char path[FS_MAX_PATH];
            std::snprintf(path, FS_MAX_PATH, "%s:/%s", device_name, GetRelativePathStart(relative_path));
            FixFileSystemPath(path);
            return fopen(path, "rb");
        }

        FILE *OpenLooseSdFile(ncm::TitleId title_id, const char *relative_path) {
            /* Allow nullptr relative path -- those are simply not openable. */
            if (relative_path == nullptr) {
                return nullptr;
            }

            char path[FS_MAX_PATH];
            std::snprintf(path, FS_MAX_PATH, "/atmosphere/titles/%016lx/exefs/%s", static_cast<u64>(title_id), GetRelativePathStart(relative_path));
            FixFileSystemPath(path);
            return OpenFile(SdCardFileSystemDeviceName, path);
        }

        bool IsFileStubbed(ncm::TitleId title_id, const char *relative_path) {
            /* Allow nullptr relative path -- those are simply not openable. */
            if (relative_path == nullptr) {
                return true;
            }

            /* Only allow stubbing in the case where we're considering SD card content. */
            if (!ShouldOverrideWithSd(title_id)) {
                return false;
            }

            char path[FS_MAX_PATH];
            std::snprintf(path, FS_MAX_PATH, "/atmosphere/titles/%016lx/exefs/%s.stub", static_cast<u64>(title_id), GetRelativePathStart(relative_path));
            FixFileSystemPath(path);
            FILE *f = OpenFile(SdCardFileSystemDeviceName, path);
            if (f == nullptr) {
                return false;
            }
            fclose(f);
            return true;
        }

        FILE *OpenBaseExefsFile(ncm::TitleId title_id, const char *relative_path) {
            /* Allow nullptr relative path -- those are simply not openable. */
            if (relative_path == nullptr) {
                return nullptr;
            }

            /* Check if stubbed. */
            if (IsFileStubbed(title_id, relative_path)) {
                return nullptr;
            }

            return OpenFile(CodeFileSystemDeviceName, relative_path);
        }

    }

    /* ScopedCodeMount functionality. */
    ScopedCodeMount::ScopedCodeMount(const ncm::TitleLocation &loc) : is_code_mounted(false), is_hbl_mounted(false) {
        this->result = this->Initialize(loc);
    }

    ScopedCodeMount::~ScopedCodeMount() {
        /* Unmount devices. */
        if (this->is_code_mounted) {
            fsdevUnmountDevice(CodeFileSystemDeviceName);
        }
        if (this->is_hbl_mounted) {
            fsdevUnmountDevice(HblFileSystemDeviceName);
        }

        /* Unmounting code means we should invalidate our configuration cache. */
        InvalidateShouldOverrideCache();
    }

    Result ScopedCodeMount::MountCodeFileSystem(const ncm::TitleLocation &loc) {
        char path[FS_MAX_PATH];

        /* Try to get the content path. */
        R_TRY(ResolveContentPath(path, loc));

        /* Try to mount the content path. */
        FsFileSystem fs;
        R_TRY(fsldrOpenCodeFileSystem(static_cast<u64>(loc.title_id), path, &fs));
        if(fsdevMountDevice(CodeFileSystemDeviceName, fs) == -1) {
            std::abort();
        }

        /* Note that we mounted code. */
        this->is_code_mounted = true;
        return ResultSuccess;
    }

    Result ScopedCodeMount::MountSdCardCodeFileSystem(const ncm::TitleLocation &loc) {
        char path[FS_MAX_PATH];

        /* Print and fix path. */
        std::snprintf(path, FS_MAX_PATH, "%s:/atmosphere/titles/%016lx/exefs.nsp", SdCardStorageMountPoint, static_cast<u64>(loc.title_id));
        FixFileSystemPath(path);
        R_TRY(MountNspFileSystem(CodeFileSystemDeviceName, path));

        /* Note that we mounted code. */
        this->is_code_mounted = true;
        return ResultSuccess;
    }

    Result ScopedCodeMount::MountHblFileSystem() {
        char path[FS_MAX_PATH];

        /* Print and fix path. */
        std::snprintf(path, FS_MAX_PATH, "%s:/%s", SdCardStorageMountPoint, GetRelativePathStart(cfg::GetHblPath()));
        FixFileSystemPath(path);
        R_TRY(MountNspFileSystem(HblFileSystemDeviceName, path));

        /* Note that we mounted HBL. */
        this->is_hbl_mounted = true;
        return ResultSuccess;
    }


    Result ScopedCodeMount::Initialize(const ncm::TitleLocation &loc) {
        bool is_sd_initialized = cfg::IsSdCardInitialized();

        /* Check if we're ready to mount the SD card. */
        if (!g_has_mounted_sd_card) {
            if (is_sd_initialized) {
                R_ASSERT(MountSdCardFileSystem());
                g_has_mounted_sd_card = true;
            }
        }

        /* Check if we should override contents. */
        if (ShouldOverrideWithHbl(loc.title_id)) {
            /* Try to mount HBL. */
            this->MountHblFileSystem();
        }
        if (ShouldOverrideWithSd(loc.title_id)) {
            /* Try to mount Code NSP on SD. */
            this->MountSdCardCodeFileSystem(loc);
        }

        /* If we haven't already mounted code, mount it. */
        if (!this->IsCodeMounted()) {
            R_TRY(this->MountCodeFileSystem(loc));
        }

        return ResultSuccess;
    }

    Result OpenCodeFile(FILE *&out, ncm::TitleId title_id, const char *relative_path) {
        FILE *f = nullptr;
        const char *ecs_device_name = ecs::Get(title_id);

        if (ecs_device_name != nullptr) {
            /* First priority: Open from external content. */
            f = OpenFile(ecs_device_name, relative_path);
        } else if (ShouldOverrideWithHbl(title_id)) {
            /* Next, try to open from HBL. */
            f = OpenFile(HblFileSystemDeviceName, relative_path);
        } else {
            /* If not ECS or HBL, try a loose file on the SD. */
            if (ShouldOverrideWithSd(title_id)) {
                f = OpenLooseSdFile(title_id, relative_path);
            }

            /* If we fail, try the original exefs. */
            if (f == nullptr) {
                f = OpenBaseExefsFile(title_id, relative_path);
            }
        }

        /* If nothing worked, we failed to find the path. */
        if (f == nullptr) {
            return ResultFsPathNotFound;
        }

        out = f;
        return ResultSuccess;
    }

    Result OpenCodeFileFromBaseExefs(FILE *&out, ncm::TitleId title_id, const char *relative_path) {
        /* Open the file. */
        FILE *f = OpenBaseExefsFile(title_id, relative_path);
        if (f == nullptr) {
            return ResultFsPathNotFound;
        }

        out = f;
        return ResultSuccess;
    }

    /* Redirection API. */
    Result ResolveContentPath(char *out_path, const ncm::TitleLocation &loc) {
        char path[FS_MAX_PATH];

        /* Try to get the path from the registered resolver. */
        LrRegisteredLocationResolver reg;
        R_TRY(lrOpenRegisteredLocationResolver(&reg));
        ON_SCOPE_EXIT { serviceClose(&reg.s); };

        R_TRY_CATCH(lrRegLrResolveProgramPath(&reg, static_cast<u64>(loc.title_id), path)) {
            R_CATCH(ResultLrProgramNotFound) {
                /* Program wasn't found via registered resolver, fall back to the normal resolver. */
                LrLocationResolver lr;
                R_TRY(lrOpenLocationResolver(static_cast<FsStorageId>(loc.storage_id), &lr));
                ON_SCOPE_EXIT { serviceClose(&lr.s); };

                R_TRY(lrLrResolveProgramPath(&lr, static_cast<u64>(loc.title_id), path));
            }
        } R_END_TRY_CATCH;

        std::strncpy(out_path, path, FS_MAX_PATH);
        out_path[FS_MAX_PATH - 1] = '\0';
        FixFileSystemPath(out_path);
        return ResultSuccess;
    }

    Result RedirectContentPath(const char *path, const ncm::TitleLocation &loc) {
        LrLocationResolver lr;
        R_TRY(lrOpenLocationResolver(static_cast<FsStorageId>(loc.storage_id), &lr));
        ON_SCOPE_EXIT { serviceClose(&lr.s); };

        return lrLrRedirectProgramPath(&lr, static_cast<u64>(loc.title_id), path);
    }

    Result RedirectHtmlDocumentPathForHbl(const ncm::TitleLocation &loc) {
        char path[FS_MAX_PATH];

        /* Open a location resolver. */
        LrLocationResolver lr;
        R_TRY(lrOpenLocationResolver(static_cast<FsStorageId>(loc.storage_id), &lr));
        ON_SCOPE_EXIT { serviceClose(&lr.s); };

        /* If there's already a Html Document path, we don't need to set one. */
        if (R_SUCCEEDED(lrLrResolveApplicationHtmlDocumentPath(&lr, static_cast<u64>(loc.title_id), path))) {
            return ResultSuccess;
        }

        /* We just need to set this to any valid NCA path. Let's use the executable path. */
        R_TRY(lrLrResolveProgramPath(&lr, static_cast<u64>(loc.title_id), path));
        R_TRY(lrLrRedirectApplicationHtmlDocumentPath(&lr, static_cast<u64>(loc.title_id), static_cast<u64>(loc.title_id), path));

        return ResultSuccess;
    }

}
