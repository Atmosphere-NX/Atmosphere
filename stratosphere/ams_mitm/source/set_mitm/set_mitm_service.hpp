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
#include <stratosphere.hpp>

namespace ams::mitm::settings {

    class SetMitmService  : public sf::IMitmServiceObject {
        private:
            enum class CommandId {
                GetLanguageCode = 0,
                GetRegionCode   = 4,
            };
        private:
            os::Mutex lock;
            cfg::OverrideLocale locale;
            bool got_locale;
        public:
            static bool ShouldMitm(const sm::MitmProcessInfo &client_info) {
                /* We will mitm:
                 * - ns and games, to allow for overriding game locales.
                 */
                const bool is_game = (ncm::IsApplicationProgramId(client_info.program_id) && !client_info.override_status.IsHbl());
                return client_info.program_id == ncm::ProgramId::Ns || is_game;
            }
        public:
            SF_MITM_SERVICE_OBJECT_CTOR(SetMitmService) {
                this->got_locale = false;
            }
        private:
            Result EnsureLocale();
        protected:
            Result GetLanguageCode(sf::Out<ams::settings::LanguageCode> out);
            Result GetRegionCode(sf::Out<ams::settings::RegionCode> out);
        public:
            DEFINE_SERVICE_DISPATCH_TABLE {
                MAKE_SERVICE_COMMAND_META(GetLanguageCode),
                MAKE_SERVICE_COMMAND_META(GetRegionCode),
            };
    };

}
