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

namespace ams::mitm::bpc {

    class BpcMitmService  : public sf::IMitmServiceObject {
        private:
            enum class CommandId {
                ShutdownSystem = 0,
                RebootSystem   = 1,
            };
        public:
            static bool ShouldMitm(const sm::MitmProcessInfo &client_info) {
                /* We will mitm:
                 * - am, to intercept the Reboot/Power buttons in the overlay menu.
                 * - fatal, to simplify payload reboot logic significantly
                 * - hbl, to allow homebrew to take advantage of the feature.
                 */
                return client_info.program_id == ncm::SystemProgramId::Am ||
                       client_info.program_id == ncm::SystemProgramId::Fatal ||
                       client_info.override_status.IsHbl();
            }
        public:
            SF_MITM_SERVICE_OBJECT_CTOR(BpcMitmService) { /* ... */ }
        protected:
            /* Overridden commands. */
            Result ShutdownSystem();
            Result RebootSystem();
        public:
            DEFINE_SERVICE_DISPATCH_TABLE {
                MAKE_SERVICE_COMMAND_META(ShutdownSystem),
                MAKE_SERVICE_COMMAND_META(RebootSystem),
            };
    };

}
