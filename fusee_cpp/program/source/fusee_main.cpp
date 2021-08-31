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
#include <exosphere.hpp>
#include "fusee_secure_initialize.hpp"
#include "fusee_sdram.hpp"

namespace ams::nxboot {

    void Main() {
        /* Perform secure hardware initialization. */
        SecureInitialize(true);

        /* Initialize Sdram. */
        InitializeSdram();

        /* TODO */
        AMS_INFINITE_LOOP();
    }

}
