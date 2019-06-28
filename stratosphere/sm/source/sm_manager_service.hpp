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
#include <stratosphere/sm.hpp>

namespace sts::sm {

    /* Service definition. */
    class ManagerService final : public IServiceObject {
        protected:
            /* Command IDs. */
            enum class CommandId {
                RegisterProcess         = 0,
                UnregisterProcess       = 1,

                AtmosphereEndInitDefers = 65000,
                AtmosphereHasMitm       = 65001,
            };
        private:
            /* Actual commands. */
            virtual Result RegisterProcess(u64 pid, InBuffer<u8> acid_sac, InBuffer<u8> aci0_sac);
            virtual Result UnregisterProcess(u64 pid);
            virtual void AtmosphereEndInitDefers();
            virtual void AtmosphereHasMitm(Out<bool> out, ServiceName service);
        public:
            DEFINE_SERVICE_DISPATCH_TABLE {
                MAKE_SERVICE_COMMAND_META(ManagerService, RegisterProcess),
                MAKE_SERVICE_COMMAND_META(ManagerService, UnregisterProcess),

                MAKE_SERVICE_COMMAND_META(ManagerService, AtmosphereEndInitDefers),
                MAKE_SERVICE_COMMAND_META(ManagerService, AtmosphereHasMitm),
            };
    };

}
