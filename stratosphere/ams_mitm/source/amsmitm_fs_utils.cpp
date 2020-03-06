/*
 * Copyright (c) 2018-2020 Atmosphère-NX
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
#include <stratosphere.hpp>
#include "amsmitm_initialization.hpp"
#include "amsmitm_fs_utils.hpp"

namespace ams::mitm::fs {

    using namespace ams::fs;

    namespace {

        /* Globals. */
        FsFileSystem g_sd_filesystem;

        /* Helpers. */
        Result EnsureSdInitialized() {
            R_UNLESS(serviceIsActive(&g_sd_filesystem.s), ams::fs::ResultSdCardNotPresent());
            return ResultSuccess();
        }

        void FormatAtmosphereRomfsPath(char *dst_path, size_t dst_path_size, ncm::ProgramId program_id, const char *src_path) {
            return FormatAtmosphereSdPath(dst_path, dst_path_size, program_id, "romfs", src_path);
        }

    }

    void OpenGlobalSdCardFileSystem() {
        R_ABORT_UNLESS(fsOpenSdCardFileSystem(&g_sd_filesystem));
    }

    Result CreateSdFile(const char *path, s64 size, s32 option) {
        R_TRY(EnsureSdInitialized());
        return fsFsCreateFile(&g_sd_filesystem, path, size, option);
    }

    Result CreateAtmosphereSdFile(const char *path, s64 size, s32 option) {
        char fixed_path[ams::fs::EntryNameLengthMax + 1];
        FormatAtmosphereSdPath(fixed_path, sizeof(fixed_path), path);
        return CreateSdFile(fixed_path, size, option);
    }

    Result OpenSdFile(FsFile *out, const char *path, u32 mode) {
        R_TRY(EnsureSdInitialized());
        return fsFsOpenFile(&g_sd_filesystem, path, mode, out);
    }

    Result OpenAtmosphereSdFile(FsFile *out, const char *path, u32 mode) {
        char fixed_path[ams::fs::EntryNameLengthMax + 1];
        FormatAtmosphereSdPath(fixed_path, sizeof(fixed_path), path);
        return OpenSdFile(out, fixed_path, mode);
    }

    Result OpenAtmosphereSdFile(FsFile *out, ncm::ProgramId program_id, const char *path, u32 mode) {
        char fixed_path[ams::fs::EntryNameLengthMax + 1];
        FormatAtmosphereSdPath(fixed_path, sizeof(fixed_path), program_id, path);
        return OpenSdFile(out, fixed_path, mode);
    }

    Result OpenAtmosphereSdRomfsFile(FsFile *out, ncm::ProgramId program_id, const char *path, u32 mode) {
        char fixed_path[ams::fs::EntryNameLengthMax + 1];
        FormatAtmosphereRomfsPath(fixed_path, sizeof(fixed_path), program_id, path);
        return OpenSdFile(out, fixed_path, mode);
    }

    Result OpenAtmosphereRomfsFile(FsFile *out, ncm::ProgramId program_id, const char *path, u32 mode, FsFileSystem *fs) {
        char fixed_path[ams::fs::EntryNameLengthMax + 1];
        FormatAtmosphereRomfsPath(fixed_path, sizeof(fixed_path), program_id, path);
        return fsFsOpenFile(fs, fixed_path, mode, out);
    }

    Result CreateSdDirectory(const char *path) {
        R_TRY(EnsureSdInitialized());
        return fsFsCreateDirectory(&g_sd_filesystem, path);
    }

    Result CreateAtmosphereSdDirectory(const char *path) {
        char fixed_path[ams::fs::EntryNameLengthMax + 1];
        FormatAtmosphereSdPath(fixed_path, sizeof(fixed_path), path);
        return CreateSdDirectory(fixed_path);
    }

    Result OpenSdDirectory(FsDir *out, const char *path, u32 mode) {
        R_TRY(EnsureSdInitialized());
        return fsFsOpenDirectory(&g_sd_filesystem, path, mode, out);
    }

    Result OpenAtmosphereSdDirectory(FsDir *out, const char *path, u32 mode) {
        char fixed_path[ams::fs::EntryNameLengthMax + 1];
        FormatAtmosphereSdPath(fixed_path, sizeof(fixed_path), path);
        return OpenSdDirectory(out, fixed_path, mode);
    }

    Result OpenAtmosphereSdDirectory(FsDir *out, ncm::ProgramId program_id, const char *path, u32 mode) {
        char fixed_path[ams::fs::EntryNameLengthMax + 1];
        FormatAtmosphereSdPath(fixed_path, sizeof(fixed_path), program_id, path);
        return OpenSdDirectory(out, fixed_path, mode);
    }

    Result OpenAtmosphereSdRomfsDirectory(FsDir *out, ncm::ProgramId program_id, const char *path, u32 mode) {
        char fixed_path[ams::fs::EntryNameLengthMax + 1];
        FormatAtmosphereRomfsPath(fixed_path, sizeof(fixed_path), program_id, path);
        return OpenSdDirectory(out, fixed_path, mode);
    }

    Result OpenAtmosphereRomfsDirectory(FsDir *out, ncm::ProgramId program_id, const char *path, u32 mode, FsFileSystem *fs) {
        char fixed_path[ams::fs::EntryNameLengthMax + 1];
        FormatAtmosphereRomfsPath(fixed_path, sizeof(fixed_path), program_id, path);
        return fsFsOpenDirectory(fs, fixed_path, mode, out);
    }

    void FormatAtmosphereSdPath(char *dst_path, size_t dst_path_size, const char *src_path) {
        if (src_path[0] == '/') {
            std::snprintf(dst_path, dst_path_size, "/atmosphere%s", src_path);
        } else {
            std::snprintf(dst_path, dst_path_size, "/atmosphere/%s", src_path);
        }
    }

    void FormatAtmosphereSdPath(char *dst_path, size_t dst_path_size, const char *subdir, const char *src_path) {
        if (src_path[0] == '/') {
            std::snprintf(dst_path, dst_path_size, "/atmosphere/%s%s", subdir, src_path);
        } else {
            std::snprintf(dst_path, dst_path_size, "/atmosphere/%s/%s", subdir, src_path);
        }
    }

    void FormatAtmosphereSdPath(char *dst_path, size_t dst_path_size, ncm::ProgramId program_id, const char *src_path) {
        if (src_path[0] == '/') {
            std::snprintf(dst_path, dst_path_size, "/atmosphere/contents/%016lx%s", static_cast<u64>(program_id), src_path);
        } else {
            std::snprintf(dst_path, dst_path_size, "/atmosphere/contents/%016lx/%s", static_cast<u64>(program_id), src_path);
        }
    }

    void FormatAtmosphereSdPath(char *dst_path, size_t dst_path_size, ncm::ProgramId program_id, const char *subdir, const char *src_path) {
        if (src_path[0] == '/') {
            std::snprintf(dst_path, dst_path_size, "/atmosphere/contents/%016lx/%s%s", static_cast<u64>(program_id), subdir, src_path);
        } else {
            std::snprintf(dst_path, dst_path_size, "/atmosphere/contents/%016lx/%s/%s", static_cast<u64>(program_id), subdir, src_path);
        }
    }

    bool HasSdRomfsContent(ncm::ProgramId program_id) {
        /* Check if romfs.bin is present. */
        {
            FsFile romfs_file;
            if (R_SUCCEEDED(OpenAtmosphereSdFile(&romfs_file, program_id, "romfs.bin", OpenMode_Read))) {
                fsFileClose(&romfs_file);
                return true;
            }
        }

        /* Check for romfs folder with content. */
        FsDir romfs_dir;
        if (R_FAILED(OpenAtmosphereSdRomfsDirectory(&romfs_dir, program_id, "", OpenDirectoryMode_All))) {
            return false;
        }
        ON_SCOPE_EXIT { fsDirClose(&romfs_dir); };

        /* Verify the folder has at least one entry. */
        s64 num_entries = 0;
        return R_SUCCEEDED(fsDirGetEntryCount(&romfs_dir, &num_entries)) && num_entries > 0;
    }

    Result SaveAtmosphereSdFile(FsFile *out, ncm::ProgramId program_id, const char *path, void *data, size_t size) {
        R_TRY(EnsureSdInitialized());

        char fixed_path[ams::fs::EntryNameLengthMax + 1];
        FormatAtmosphereSdPath(fixed_path, sizeof(fixed_path), program_id, path);

        /* Unconditionally create. */
        /* Don't check error, as a failure here should be okay. */
        FsFile f;
        fsFsCreateFile(&g_sd_filesystem, fixed_path, size, 0);

        /* Try to open. */
        R_TRY(fsFsOpenFile(&g_sd_filesystem, fixed_path, OpenMode_ReadWrite, &f));
        auto file_guard = SCOPE_GUARD { fsFileClose(&f); };

        /* Try to set the size. */
        R_TRY(fsFileSetSize(&f, static_cast<s64>(size)));

        /* Try to write data. */
        R_TRY(fsFileWrite(&f, 0, data, size, FsWriteOption_Flush));

        /* Set output. */
        file_guard.Cancel();
        *out = f;
        return ResultSuccess();
    }

    Result CreateAndOpenAtmosphereSdFile(FsFile *out, ncm::ProgramId program_id, const char *path, size_t size) {
        R_TRY(EnsureSdInitialized());

        char fixed_path[ams::fs::EntryNameLengthMax + 1];
        FormatAtmosphereSdPath(fixed_path, sizeof(fixed_path), program_id, path);

        /* Unconditionally create. */
        /* Don't check error, as a failure here should be okay. */
        FsFile f;
        fsFsCreateFile(&g_sd_filesystem, fixed_path, size, 0);

        /* Try to open. */
        R_TRY(fsFsOpenFile(&g_sd_filesystem, fixed_path, OpenMode_ReadWrite, &f));
        auto file_guard = SCOPE_GUARD { fsFileClose(&f); };

        /* Try to set the size. */
        R_TRY(fsFileSetSize(&f, static_cast<s64>(size)));

        /* Set output. */
        file_guard.Cancel();
        *out = f;
        return ResultSuccess();

    }

}
