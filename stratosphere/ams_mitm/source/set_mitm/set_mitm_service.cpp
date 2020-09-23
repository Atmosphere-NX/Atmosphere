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
#include "set_mitm_service.hpp"
#include "set_shim.h"

namespace ams::mitm::settings {

    using namespace ams::settings;

    namespace {

        constinit os::ProcessId g_application_process_id = os::InvalidProcessId;
        constinit cfg::OverrideLocale g_application_locale;
        constinit bool g_valid_language;
        constinit bool g_valid_region;
    }

    SetMitmService::SetMitmService(std::shared_ptr<::Service> &&s, const sm::MitmProcessInfo &c) : sf::MitmServiceImplBase(std::forward<std::shared_ptr<::Service>>(s), c) {
        if (this->client_info.program_id == ncm::SystemProgramId::Ns) {
            os::ProcessId application_process_id;
            if (R_SUCCEEDED(pm::dmnt::GetApplicationProcessId(std::addressof(application_process_id))) && g_application_process_id == application_process_id) {
                this->locale = g_application_locale;
                this->is_valid_language = g_valid_language;
                this->is_valid_region   = g_valid_region;
                this->got_locale = true;
            } else {
                this->InvalidateLocale();
            }
        } else {
            this->InvalidateLocale();
        }
    }

    Result SetMitmService::EnsureLocale() {
        /* Optimization: if locale has already been gotten, we can stop. */
        if (AMS_LIKELY(this->got_locale)) {
            return ResultSuccess();
        }

        std::scoped_lock lk(this->lock);

        const bool is_ns = this->client_info.program_id == ncm::SystemProgramId::Ns;

        if (!this->got_locale) {
            ncm::ProgramId program_id = this->client_info.program_id;
            os::ProcessId application_process_id = os::InvalidProcessId;

            if (is_ns) {
                /* When NS asks for a locale, refresh to get the current application locale. */
                R_TRY(pm::dmnt::GetApplicationProcessId(std::addressof(application_process_id)));
                R_TRY(pm::info::GetProgramId(std::addressof(program_id), application_process_id));
            }
            this->locale            = cfg::GetOverrideLocale(program_id);
            this->is_valid_language = settings::IsValidLanguageCode(this->locale.language_code);
            this->is_valid_region   = settings::IsValidRegionCode(this->locale.region_code);
            this->got_locale        = true;

            if (is_ns) {
                g_application_locale     = this->locale;
                g_valid_language         = this->is_valid_language;
                g_valid_region           = this->is_valid_region;
                g_application_process_id = application_process_id;
            }
        }

        return ResultSuccess();
    }

    void SetMitmService::InvalidateLocale() {
        std::scoped_lock lk(this->lock);

        std::memset(std::addressof(this->locale), 0xCC, sizeof(this->locale));
        this->is_valid_language = false;
        this->is_valid_region   = false;
        this->got_locale        = false;
    }

    Result SetMitmService::GetLanguageCode(sf::Out<settings::LanguageCode> out) {
        this->EnsureLocale();

        /* If there's no override locale, just use the actual one. */
        if (AMS_UNLIKELY(!this->is_valid_language)) {
            static_assert(sizeof(u64) == sizeof(settings::LanguageCode));
            R_TRY(setGetLanguageCodeFwd(this->forward_service.get(), reinterpret_cast<u64 *>(std::addressof(this->locale.language_code))));

            this->is_valid_language = true;
            if (this->client_info.program_id == ncm::SystemProgramId::Ns) {
                g_application_locale.language_code = this->locale.language_code;
                g_valid_language                   = true;
            }
        }

        out.SetValue(this->locale.language_code);
        return ResultSuccess();
    }

    Result SetMitmService::GetRegionCode(sf::Out<settings::RegionCode> out) {
        this->EnsureLocale();

        /* If there's no override locale, just use the actual one. */
        if (AMS_UNLIKELY(!this->is_valid_region)) {
            static_assert(sizeof(::SetRegion) == sizeof(settings::RegionCode));
            R_TRY(setGetRegionCodeFwd(this->forward_service.get(), reinterpret_cast<::SetRegion *>(std::addressof(this->locale.region_code))));

            this->is_valid_region = true;
            if (this->client_info.program_id == ncm::SystemProgramId::Ns) {
                g_application_locale.region_code = this->locale.region_code;
                g_valid_region                   = true;
            }
        }

        out.SetValue(this->locale.region_code);
        return ResultSuccess();
    }

}
