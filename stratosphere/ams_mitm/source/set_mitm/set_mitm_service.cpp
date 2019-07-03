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

#include <mutex>
#include <algorithm>
#include <switch.h>
#include "set_mitm_service.hpp"
#include "set_shim.h"

void SetMitmService::PostProcess(IMitmServiceObject *obj, IpcResponseContext *ctx) {
    /* No commands need postprocessing. */
}

bool SetMitmService::IsValidLanguageCode(u64 lang_code) {
    static constexpr u64 s_valid_language_codes[] = {
        LanguageCode_Japanese,
        LanguageCode_AmericanEnglish,
        LanguageCode_French,
        LanguageCode_German,
        LanguageCode_Italian,
        LanguageCode_Spanish,
        LanguageCode_Chinese,
        LanguageCode_Korean,
        LanguageCode_Dutch,
        LanguageCode_Portuguese,
        LanguageCode_Russian,
        LanguageCode_Taiwanese,
        LanguageCode_BritishEnglish,
        LanguageCode_CanadianFrench,
        LanguageCode_LatinAmericanSpanish,
        LanguageCode_SimplifiedChinese,
        LanguageCode_TraditionalChinese,
    };
    size_t num_language_codes = sts::util::size(s_valid_language_codes);
    if (GetRuntimeFirmwareVersion() < FirmwareVersion_400) {
        /* 4.0.0 added simplified and traditional chinese. */
        num_language_codes -= 2;
    }

    for (size_t i = 0; i < num_language_codes; i++) {
        if (lang_code == s_valid_language_codes[i]) {
            return true;
        }
    }

    return false;
}

bool SetMitmService::IsValidRegionCode(u32 region_code) {
    return region_code < RegionCode_Max;
}

Result SetMitmService::EnsureLocale() {
    std::scoped_lock<HosMutex> lk(this->lock);

    if (!this->got_locale) {
        std::memset(&this->locale, 0xCC, sizeof(this->locale));
        if (this->title_id == sts::ncm::TitleId::Ns) {
            u64 app_pid = 0;
            u64 app_tid = 0;
            R_TRY(pmdmntGetApplicationPid(&app_pid));
            R_TRY(pminfoGetTitleId(&app_tid, app_pid));
            this->locale = Utils::GetTitleOverrideLocale(app_tid);
        } else {
            this->locale = Utils::GetTitleOverrideLocale(static_cast<u64>(this->title_id));
            this->got_locale = true;
        }
    }

    return ResultSuccess;
}

Result SetMitmService::GetLanguageCode(Out<u64> out_lang_code) {
    this->EnsureLocale();

    if (!IsValidLanguageCode(this->locale.language_code)) {
        return ResultAtmosphereMitmShouldForwardToSession;
    }

    out_lang_code.SetValue(this->locale.language_code);
    return ResultSuccess;
}

Result SetMitmService::GetRegionCode(Out<u32> out_region_code) {
    this->EnsureLocale();

    if (!IsValidRegionCode(this->locale.region_code)) {
        return ResultAtmosphereMitmShouldForwardToSession;
    }

    out_region_code.SetValue(this->locale.region_code);
    return ResultSuccess;
}

Result SetMitmService::GetAvailableLanguageCodes(OutPointerWithClientSize<u64> out_language_codes, Out<s32> out_count) {
    return setGetAvailableLanguageCodesFwd(this->forward_service.get(), out_count.GetPointer(), out_language_codes.pointer, out_language_codes.num_elements);
}
