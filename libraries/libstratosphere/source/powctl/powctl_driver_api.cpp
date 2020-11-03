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
#include "impl/powctl_select_board_driver.hpp"
#include "impl/powctl_device_management.hpp"

namespace ams::powctl {

    void Initialize(bool enable_interrupt_handlers) {
        /* Initialize the board driver. */
        impl::board::Initialize(enable_interrupt_handlers);

        /* Initialize drivers. */
        impl::InitializeDrivers();

    }

    void Finalize() {
        /* Finalize drivers. */
        impl::FinalizeDrivers();

        /* Finalize the board driver. */
        impl::board::Finalize();
    }

}
