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

struct OverrideKey {
    u64 key_combination;
    bool override_by_default;
};

class Utils {
    public:
        static bool IsSdInitialized();
        static void WaitSdInitialized();

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

        /* Delayed Initialization + MitM detection. */
        static void InitializeThreadFunc(void *args);

        static bool IsHblTid(u64 tid);
        static bool IsWebAppletTid(u64 tid);

        static bool HasTitleFlag(u64 tid, const char *flag);
        static bool HasHblFlag(const char *flag);
        static bool HasGlobalFlag(const char *flag);
        static bool HasFlag(u64 tid, const char *flag);

        static bool HasSdMitMFlag(u64 tid);
        static bool HasSdDisableMitMFlag(u64 tid);


        static bool IsHidAvailable();
        static Result GetKeysHeld(u64 *keys);

        static OverrideKey GetTitleOverrideKey(u64 tid);
        static bool HasOverrideButton(u64 tid);

        /* Settings! */
        static Result GetSettingsItemValueSize(const char *name, const char *key, u64 *out_size);
        static Result GetSettingsItemValue(const char *name, const char *key, void *out, size_t max_size, u64 *out_size);

        static Result GetSettingsItemBooleanValue(const char *name, const char *key, bool *out);
        
        /* Error occurred. */
        static void RebootToFatalError(AtmosphereFatalErrorContext *ctx);
    private:
        static void RefreshConfiguration();
};