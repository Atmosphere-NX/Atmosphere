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
#include <stratosphere.hpp>

namespace ams::mitm::settings {

    namespace {

        #define AMS_SETTINGS_MITM_INTERFACE_INFO(C, H)                                                       \
            AMS_SF_METHOD_INFO(C, H, 0, Result, GetLanguageCode, (sf::Out<ams::settings::LanguageCode> out)) \
            AMS_SF_METHOD_INFO(C, H, 4, Result, GetRegionCode,   (sf::Out<ams::settings::RegionCode> out))

        AMS_SF_DEFINE_MITM_INTERFACE(ISetMitmInterface, AMS_SETTINGS_MITM_INTERFACE_INFO)

    }

    class SetMitmService : public sf::MitmServiceImplBase {
        private:
            os::Mutex lock{false};
            cfg::OverrideLocale locale;
            bool got_locale = false;
            bool is_valid_language = false;
            bool is_valid_region   = false;
        public:
            SetMitmService(std::shared_ptr<::Service> &&s, const sm::MitmProcessInfo &c);
        public:
            static bool ShouldMitm(const sm::MitmProcessInfo &client_info) {
                /* We will mitm:
                 * - ns and games, to allow for overriding game locales.
                 */
                const bool is_game = (ncm::IsApplicationId(client_info.program_id) && !client_info.override_status.IsHbl());
                return client_info.program_id == ncm::SystemProgramId::Ns || is_game;
            }
        private:
            void InvalidateLocale();
            Result EnsureLocale();
        public:
            Result GetLanguageCode(sf::Out<ams::settings::LanguageCode> out);
            Result GetRegionCode(sf::Out<ams::settings::RegionCode> out);
    };
    static_assert(IsISetMitmInterface<SetMitmService>);

}
