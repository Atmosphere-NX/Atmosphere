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

namespace ams::mitm::hid {

    class HidMitmService  : public sf::IMitmServiceObject {
        private:
            enum class CommandId {
                SetSupportedNpadStyleSet = 100,
            };
        public:
            static bool ShouldMitm(const sm::MitmProcessInfo &client_info) {
                /* TODO: Remove in Atmosphere 0.10.2. */
                /* We will mitm:
                 * - hbl, to help homebrew not need to be recompiled.
                 */
                return client_info.override_status.IsHbl();
            }
        public:
            SF_MITM_SERVICE_OBJECT_CTOR(HidMitmService) { /* ... */ }
        protected:
            /* Overridden commands. */
            Result SetSupportedNpadStyleSet(const sf::ClientAppletResourceUserId &client_aruid, u32 style_set);
        public:
            DEFINE_SERVICE_DISPATCH_TABLE {
                MAKE_SERVICE_COMMAND_META(SetSupportedNpadStyleSet),
            };
    };

}
