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
 
#pragma once
#include <switch.h>
#include <stratosphere.hpp>

class Utils {
    public:
        static bool IsSdInitialized();
        static Result OpenSdFile(const char *fn, int flags, FsFile *out);
        static Result OpenSdFileForAtmosphere(u64 title_id, const char *fn, int flags, FsFile *out);
        static Result OpenRomFSSdFile(u64 title_id, const char *fn, int flags, FsFile *out);
        static Result OpenSdDir(const char *path, FsDir *out);
        static Result OpenSdDirForAtmosphere(u64 title_id, const char *path, FsDir *out);
        static Result OpenRomFSSdDir(u64 title_id, const char *path, FsDir *out);
        
        
        static Result OpenRomFSFile(FsFileSystem *fs, u64 title_id, const char *fn, int flags, FsFile *out);
        static Result OpenRomFSDir(FsFileSystem *fs, u64 title_id, const char *path, FsDir *out);
        
        static Result HasSdRomfsContent(u64 title_id, bool *out);
        
        /* SD card Initialization + MitM detection. */
        static void InitializeSdThreadFunc(void *args);
        static bool HasSdMitMFlag(u64 tid);
};