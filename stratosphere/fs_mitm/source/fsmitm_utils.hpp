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

enum BisStorageId : u32 {
    BisStorageId_Boot0 = 0,
    BisStorageId_Boot1 = 10,
    BisStorageId_RawNand = 20,
    BisStorageId_BcPkg2_1 = 21,
    BisStorageId_BcPkg2_2 = 22,
    BisStorageId_BcPkg2_3 = 23,
    BisStorageId_BcPkg2_4 = 24,
    BisStorageId_BcPkg2_5 = 25,
    BisStorageId_BcPkg2_6 = 26,
    BisStorageId_Prodinfo = 27,
    BisStorageId_ProdinfoF = 28,
    BisStorageId_Safe = 29,
    BisStorageId_User = 30,
    BisStorageId_System = 31,
    BisStorageId_SystemProperEncryption = 32,
    BisStorageId_SystemProperPartition = 33,
};

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
        
        static Result SaveSdFileForAtmosphere(u64 title_id, const char *fn, void *data, size_t size);
        
        static bool HasSdRomfsContent(u64 title_id);
        
        /* SD card Initialization + MitM detection. */
        static void InitializeSdThreadFunc(void *args);
        
        static bool HasTitleFlag(u64 tid, const char *flag);
        static bool HasHblFlag(const char *flag);
        static bool HasGlobalFlag(const char *flag);
        static bool HasFlag(u64 tid, const char *flag);
        
        static bool HasSdMitMFlag(u64 tid);
        static bool HasSdDisableMitMFlag(u64 tid);
        
        
        static bool IsHidInitialized();
        static void InitializeHidThreadFunc(void *args);
        static Result GetKeysDown(u64 *keys);
        static bool HasOverrideButton(u64 tid);
    private:
        static void RefreshConfiguration();
};