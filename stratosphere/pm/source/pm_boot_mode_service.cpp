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
#include "pm_boot_mode_service.hpp"

namespace ams::pm {

    namespace {

        /* Global bootmode. */
        BootMode g_boot_mode = BootMode::Normal;

    }

    /* Override of weakly linked boot_mode_api functions. */
    namespace bm {

        BootMode GetBootMode() {
            return g_boot_mode;
        }

        void SetMaintenanceBoot() {
            g_boot_mode = BootMode::Maintenance;
        }

    }

    /* Service command implementations. */
    void BootModeService::GetBootMode(sf::Out<u32> out) {
        out.SetValue(static_cast<u32>(pm::bm::GetBootMode()));
    }

    void BootModeService::SetMaintenanceBoot() {
        pm::bm::SetMaintenanceBoot();
    }

}
