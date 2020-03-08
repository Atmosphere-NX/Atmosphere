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
#include "set_mitm_service.hpp"

namespace ams::mitm::settings {

    using namespace ams::settings;

    Result SetMitmService::EnsureLocale() {
        std::scoped_lock lk(this->lock);

        const bool is_ns = this->client_info.program_id == ncm::SystemProgramId::Ns;

        if (!this->got_locale) {
            std::memset(&this->locale, 0xCC, sizeof(this->locale));
            ncm::ProgramId program_id = this->client_info.program_id;
            if (is_ns) {
                /* When NS asks for a locale, refresh to get the current application locale. */
                os::ProcessId application_process_id;
                R_TRY(pm::dmnt::GetApplicationProcessId(&application_process_id));
                R_TRY(pm::info::GetProgramId(&program_id, application_process_id));
            }
            this->locale = cfg::GetOverrideLocale(program_id);
            this->got_locale = !is_ns;
        }

        return ResultSuccess();
    }

    Result SetMitmService::GetLanguageCode(sf::Out<settings::LanguageCode> out) {
        this->EnsureLocale();

        /* If there's no override locale, just use the actual one. */
        R_UNLESS(settings::IsValidLanguageCode(this->locale.language_code), sm::mitm::ResultShouldForwardToSession());

        out.SetValue(this->locale.language_code);
        return ResultSuccess();
    }

    Result SetMitmService::GetRegionCode(sf::Out<settings::RegionCode> out) {
        this->EnsureLocale();

        /* If there's no override locale, just use the actual one. */
        R_UNLESS(settings::IsValidRegionCode(this->locale.region_code), sm::mitm::ResultShouldForwardToSession());

        out.SetValue(this->locale.region_code);
        return ResultSuccess();
    }

}
