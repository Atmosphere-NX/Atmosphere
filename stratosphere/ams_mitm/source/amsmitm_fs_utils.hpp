/*
 * Copyright (c) Atmosphère-NX
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
#pragma once
#include <stratosphere.hpp>

namespace ams::mitm::fs {

    /* Initialization. */
    void OpenGlobalSdCardFileSystem();

    /* Utilities. */
    Result DeleteAtmosphereSdFile(const char *path);
    Result CreateSdFile(const char *path, s64 size, s32 option);
    Result CreateAtmosphereSdFile(const char *path, s64 size, s32 option);
    Result OpenSdFile(FsFile *out, const char *path, u32 mode);
    Result OpenAtmosphereSdFile(FsFile *out, const char *path, u32 mode);
    Result OpenAtmosphereSdFile(FsFile *out, ncm::ProgramId program_id, const char *path, u32 mode);
    Result OpenAtmosphereSdRomfsFile(FsFile *out, ncm::ProgramId program_id, const char *path, u32 mode);
    Result OpenAtmosphereRomfsFile(FsFile *out, ncm::ProgramId program_id, const char *path, u32 mode, FsFileSystem *fs);

    bool HasSdFile(const char *path);
    bool HasAtmosphereSdFile(const char *path);

    Result CreateSdDirectory(const char *path);
    Result CreateAtmosphereSdDirectory(const char *path);
    Result OpenSdDirectory(FsDir *out, const char *path, u32 mode);
    Result OpenAtmosphereSdDirectory(FsDir *out, const char *path, u32 mode);
    Result OpenAtmosphereSdDirectory(FsDir *out, ncm::ProgramId program_id, const char *path, u32 mode);
    Result OpenAtmosphereSdRomfsDirectory(FsDir *out, ncm::ProgramId program_id, const char *path, u32 mode);
    Result OpenAtmosphereRomfsDirectory(FsDir *out, ncm::ProgramId program_id, const char *path, u32 mode, FsFileSystem *fs);

    void FormatAtmosphereSdPath(char *dst_path, size_t dst_path_size, const char *src_path);
    void FormatAtmosphereSdPath(char *dst_path, size_t dst_path_size, const char *subdir, const char *src_path);
    void FormatAtmosphereSdPath(char *dst_path, size_t dst_path_size, ncm::ProgramId program_id, const char *src_path);
    void FormatAtmosphereSdPath(char *dst_path, size_t dst_path_size, ncm::ProgramId program_id, const char *subdir, const char *src_path);

    bool HasSdRomfsContent(ncm::ProgramId program_id);

    Result SaveAtmosphereSdFile(FsFile *out, ncm::ProgramId program_id, const char *path, void *data, size_t size);
    Result CreateAndOpenAtmosphereSdFile(FsFile *out, ncm::ProgramId program_id, const char *path, size_t size);

}
