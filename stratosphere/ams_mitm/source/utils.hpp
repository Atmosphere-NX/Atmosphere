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

struct OverrideKey {
    u64 key_combination;
    bool override_by_default;
};

struct OverrideLocale {
    u64 language_code;
    u32 region_code;
};

enum RegionCode : u32 {
    RegionCode_Japan = 0,
    RegionCode_America = 1,
    RegionCode_Europe = 2,
    RegionCode_Australia = 3,
    RegionCode_China = 4,
    RegionCode_Korea = 5,
    RegionCode_Taiwan = 6,

    RegionCode_Max,
};

static constexpr inline u64 EncodeLanguageCode(const char *code) {
    u64 lang_code = 0;
    for (size_t i = 0; i < sizeof(lang_code); i++) {
        if (code[i] == '\x00') {
            break;
        }
        lang_code |= static_cast<u64>(code[i]) << (8ul * i);
    }
    return lang_code;
}

enum LanguageCode : u64 {
    LanguageCode_Japanese = EncodeLanguageCode("ja"),
    LanguageCode_AmericanEnglish = EncodeLanguageCode("en-US"),
    LanguageCode_French = EncodeLanguageCode("fr"),
    LanguageCode_German = EncodeLanguageCode("de"),
    LanguageCode_Italian = EncodeLanguageCode("it"),
    LanguageCode_Spanish = EncodeLanguageCode("es"),
    LanguageCode_Chinese = EncodeLanguageCode("zh-CN"),
    LanguageCode_Korean = EncodeLanguageCode("ko"),
    LanguageCode_Dutch = EncodeLanguageCode("nl"),
    LanguageCode_Portuguese = EncodeLanguageCode("pt"),
    LanguageCode_Russian = EncodeLanguageCode("ru"),
    LanguageCode_Taiwanese = EncodeLanguageCode("zh-TW"),
    LanguageCode_BritishEnglish = EncodeLanguageCode("en-GB"),
    LanguageCode_CanadianFrench = EncodeLanguageCode("fr-CA"),
    LanguageCode_LatinAmericanSpanish = EncodeLanguageCode("es-419"),
    /* 4.0.0+ */
    LanguageCode_SimplifiedChinese = EncodeLanguageCode("zh-Hans"),
    LanguageCode_TraditionalChinese = EncodeLanguageCode("zh-Hant"),
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
        static void WaitHidAvailable();
        static Result GetKeysHeld(u64 *keys);

        static OverrideKey GetTitleOverrideKey(u64 tid);
        static bool HasOverrideButton(u64 tid);

        static OverrideLocale GetTitleOverrideLocale(u64 tid);

        /* Settings! */
        static Result GetSettingsItemValueSize(const char *name, const char *key, u64 *out_size);
        static Result GetSettingsItemValue(const char *name, const char *key, void *out, size_t max_size, u64 *out_size);

        static Result GetSettingsItemBooleanValue(const char *name, const char *key, bool *out);

        /* Error occurred. */
        static void RebootToFatalError(AtmosphereFatalErrorContext *ctx);
    private:
        static void RefreshConfiguration();
};