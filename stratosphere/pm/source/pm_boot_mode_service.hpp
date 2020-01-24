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

namespace ams::pm::bm {

    class BootModeService final : public sf::IServiceObject {
        private:
            enum class CommandId {
                GetBootMode        = 0,
                SetMaintenanceBoot = 1,
            };
        private:
            /* Actual command implementations. */
            void GetBootMode(sf::Out<u32> out);
            void SetMaintenanceBoot();
        public:
            DEFINE_SERVICE_DISPATCH_TABLE {
                MAKE_SERVICE_COMMAND_META(GetBootMode),
                MAKE_SERVICE_COMMAND_META(SetMaintenanceBoot),
            };
    };

}
