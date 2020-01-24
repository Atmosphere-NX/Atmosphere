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

namespace ams::sm {

    /* Service definition. */
    class ManagerService final : public sf::IServiceObject {
        protected:
            /* Command IDs. */
            enum class CommandId {
                RegisterProcess           = 0,
                UnregisterProcess         = 1,

                AtmosphereEndInitDefers   = 65000,
                AtmosphereHasMitm         = 65001,
                AtmosphereRegisterProcess = 65002,
            };
        private:
            /* Actual commands. */
            virtual Result RegisterProcess(os::ProcessId process_id, const sf::InBuffer &acid_sac, const sf::InBuffer &aci_sac);
            virtual Result UnregisterProcess(os::ProcessId process_id);
            virtual void AtmosphereEndInitDefers();
            virtual void AtmosphereHasMitm(sf::Out<bool> out, ServiceName service);
            virtual Result AtmosphereRegisterProcess(os::ProcessId process_id, ncm::ProgramId program_id, cfg::OverrideStatus override_status, const sf::InBuffer &acid_sac, const sf::InBuffer &aci_sac);
        public:
            DEFINE_SERVICE_DISPATCH_TABLE {
                MAKE_SERVICE_COMMAND_META(RegisterProcess),
                MAKE_SERVICE_COMMAND_META(UnregisterProcess),

                MAKE_SERVICE_COMMAND_META(AtmosphereEndInitDefers),
                MAKE_SERVICE_COMMAND_META(AtmosphereHasMitm),
                MAKE_SERVICE_COMMAND_META(AtmosphereRegisterProcess),
            };
    };

}
