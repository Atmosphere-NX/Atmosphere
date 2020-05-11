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

#pragma once
#include <vapours.hpp>

namespace ams::settings {

    enum Language {
        Language_Japanese,
        Language_AmericanEnglish,
        Language_French,
        Language_German,
        Language_Italian,
        Language_Spanish,
        Language_Chinese,
        Language_Korean,
        Language_Dutch,
        Language_Portuguese,
        Language_Russian,
        Language_Taiwanese,
        Language_BritishEnglish,
        Language_CanadianFrench,
        Language_LatinAmericanSpanish,
        /* 4.0.0+ */
        Language_SimplifiedChinese,
        Language_TraditionalChinese,

        Language_Count,
    };

    struct LanguageCode {
        static constexpr size_t MaxLength = 8;

        char name[MaxLength];

        static constexpr LanguageCode Encode(const char *name, size_t name_size) {
            LanguageCode out{};
            for (size_t i = 0; i < MaxLength && i < name_size; i++) {
                out.name[i] = name[i];
            }
            return out;
        }

        static constexpr LanguageCode Encode(const char *name) {
            return Encode(name, std::strlen(name));
        }

        template<Language Lang>
        static constexpr inline LanguageCode EncodeLanguage = [] {
            if constexpr (false) { /* ... */ }
            #define AMS_MATCH_LANGUAGE(lang, enc) else if constexpr (Lang == Language_##lang) { return LanguageCode::Encode(enc); }
            AMS_MATCH_LANGUAGE(Japanese,                "ja")
            AMS_MATCH_LANGUAGE(AmericanEnglish,         "en-US")
            AMS_MATCH_LANGUAGE(French,                  "fr")
            AMS_MATCH_LANGUAGE(German,                  "de")
            AMS_MATCH_LANGUAGE(Italian,                 "it")
            AMS_MATCH_LANGUAGE(Spanish,                 "es")
            AMS_MATCH_LANGUAGE(Chinese,                 "zh-CN")
            AMS_MATCH_LANGUAGE(Korean,                  "ko")
            AMS_MATCH_LANGUAGE(Dutch,                   "nl")
            AMS_MATCH_LANGUAGE(Portuguese,              "pt")
            AMS_MATCH_LANGUAGE(Russian,                 "ru")
            AMS_MATCH_LANGUAGE(Taiwanese,               "zh-TW")
            AMS_MATCH_LANGUAGE(BritishEnglish,          "en-GB")
            AMS_MATCH_LANGUAGE(CanadianFrench,          "fr-CA")
            AMS_MATCH_LANGUAGE(LatinAmericanSpanish,    "es-419")
            /* 4.0.0+ */
            AMS_MATCH_LANGUAGE(SimplifiedChinese,       "zh-Hans")
            AMS_MATCH_LANGUAGE(TraditionalChinese,      "zh-Hant")
            #undef AMS_MATCH_LANGUAGE
            else { static_assert(Lang != Language_Japanese); }
        }();

        static constexpr inline LanguageCode Encode(const Language language) {
            constexpr LanguageCode EncodedLanguages[Language_Count] = {
                EncodeLanguage<Language_Japanese>,
                EncodeLanguage<Language_AmericanEnglish>,
                EncodeLanguage<Language_French>,
                EncodeLanguage<Language_German>,
                EncodeLanguage<Language_Italian>,
                EncodeLanguage<Language_Spanish>,
                EncodeLanguage<Language_Chinese>,
                EncodeLanguage<Language_Korean>,
                EncodeLanguage<Language_Dutch>,
                EncodeLanguage<Language_Portuguese>,
                EncodeLanguage<Language_Russian>,
                EncodeLanguage<Language_Taiwanese>,
                EncodeLanguage<Language_BritishEnglish>,
                EncodeLanguage<Language_CanadianFrench>,
                EncodeLanguage<Language_LatinAmericanSpanish>,
                /* 4.0.0+ */
                EncodeLanguage<Language_SimplifiedChinese>,
                EncodeLanguage<Language_TraditionalChinese>,
            };
            return EncodedLanguages[language];
        }

    };

    constexpr inline bool operator==(const LanguageCode &lhs, const LanguageCode &rhs) {
        return std::strncmp(lhs.name, rhs.name, sizeof(lhs)) == 0;
    }

    constexpr inline bool operator!=(const LanguageCode &lhs, const LanguageCode &rhs) {
        return !(lhs == rhs);
    }

    constexpr inline bool operator==(const LanguageCode &lhs, const Language &rhs) {
        return lhs == LanguageCode::Encode(rhs);
    }

    constexpr inline bool operator!=(const LanguageCode &lhs, const Language &rhs) {
        return !(lhs == rhs);
    }

    constexpr inline bool operator==(const Language &lhs, const LanguageCode &rhs) {
        return rhs == lhs;
    }

    constexpr inline bool operator!=(const Language &lhs, const LanguageCode &rhs) {
        return !(lhs == rhs);
    }

    namespace impl {

        template<size_t ...Is>
        constexpr inline bool IsValidLanguageCode(const LanguageCode &lc, std::index_sequence<Is...>) {
            return ((lc == LanguageCode::Encode(static_cast<Language>(Is))) || ...);
        }

    }

    constexpr inline bool IsValidLanguageCodeDeprecated(const LanguageCode &lc) {
        return impl::IsValidLanguageCode(lc, std::make_index_sequence<Language_Count - 2>{});
    }

    constexpr inline bool IsValidLanguageCode(const LanguageCode &lc) {
        return impl::IsValidLanguageCode(lc, std::make_index_sequence<Language_Count>{});
    }

    static_assert(util::is_pod<LanguageCode>::value);
    static_assert(sizeof(LanguageCode) == sizeof(u64));

    /* Not an official type, but convenient. */
    enum RegionCode : s32 {
        RegionCode_Japan,
        RegionCode_America,
        RegionCode_Europe,
        RegionCode_Australia,
        RegionCode_China,
        RegionCode_Korea,
        RegionCode_Taiwan,

        RegionCode_Count,
    };

    constexpr inline bool IsValidRegionCode(const RegionCode rc) {
        return 0 <= rc && rc < RegionCode_Count;
    }

    /* This needs to be defined separately from libnx's so that it can inherit from sf::LargeData. */

    struct FirmwareVersion : public sf::LargeData {
        u8 major;
        u8 minor;
        u8 micro;
        u8 padding1;
        u8 revision_major;
        u8 revision_minor;
        u8 padding2;
        u8 padding3;
        char platform[0x20];
        char version_hash[0x40];
        char display_version[0x18];
        char display_title[0x80];

        constexpr inline u32 GetVersion() const {
            return (static_cast<u32>(major) << 16) | (static_cast<u32>(minor) << 8) | (static_cast<u32>(micro) << 0);
        }
    };

    static_assert(util::is_pod<FirmwareVersion>::value);
    static_assert(sizeof(FirmwareVersion) == sizeof(::SetSysFirmwareVersion));

    constexpr inline bool operator==(const FirmwareVersion &lhs, const FirmwareVersion &rhs) {
        return lhs.GetVersion() == rhs.GetVersion();
    }

    constexpr inline bool operator!=(const FirmwareVersion &lhs, const FirmwareVersion &rhs) {
        return !(lhs == rhs);
    }

    constexpr inline bool operator<(const FirmwareVersion &lhs, const FirmwareVersion &rhs) {
        return lhs.GetVersion() < rhs.GetVersion();
    }

    constexpr inline bool operator>=(const FirmwareVersion &lhs, const FirmwareVersion &rhs) {
        return !(lhs < rhs);
    }

    constexpr inline bool operator<=(const FirmwareVersion &lhs, const FirmwareVersion &rhs) {
        return lhs.GetVersion() <= rhs.GetVersion();
    }

    constexpr inline bool operator>(const FirmwareVersion &lhs, const FirmwareVersion &rhs) {
        return !(lhs <= rhs);
    }

}
