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
#include <stratosphere/ldr.hpp>
#include <stratosphere/pm.hpp>

namespace sts::pm::info {

    class InformationService final : public sf::IServiceObject {
        private:
            enum class CommandId {
                GetTitleId                 = 0,

                AtmosphereGetProcessId     = 65000,
                AtmosphereHasLaunchedTitle = 65001,
            };
        private:
            /* Actual command implementations. */
            Result GetTitleId(sf::Out<ncm::TitleId> out, os::ProcessId process_id);

            /* Atmosphere extension commands. */
            Result AtmosphereGetProcessId(sf::Out<os::ProcessId> out, ncm::TitleId title_id);
            Result AtmosphereHasLaunchedTitle(sf::Out<bool> out, ncm::TitleId title_id);
        public:
            DEFINE_SERVICE_DISPATCH_TABLE {
                MAKE_SERVICE_COMMAND_META(GetTitleId),

                MAKE_SERVICE_COMMAND_META(AtmosphereGetProcessId),
                MAKE_SERVICE_COMMAND_META(AtmosphereHasLaunchedTitle),
            };
    };

}
