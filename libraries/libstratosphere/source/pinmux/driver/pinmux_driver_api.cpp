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
#include "pinmux_select_board_impl.hpp"

namespace ams::pinmux::driver {

    namespace {

        constinit os::SdkMutex g_init_mutex;
        constinit int g_init_count = 0;

    }

    void Initialize() {
        std::scoped_lock lk(g_init_mutex);

        if ((g_init_count++) == 0) {
            if (!board::IsInitialized()) {
                board::Initialize();
            }
        }
    }

    void Finalize() {
        std::scoped_lock lk(g_init_mutex);

        if ((--g_init_count) == 0) {
            AMS_ASSERT(board::IsInitialized());
            board::Finalize();
        }
    }

    void SetInitialConfig() {
        board::SetInitialConfig();
    }

    void SetInitialDrivePadConfig() {
        board::SetInitialDrivePadConfig();
    }

}
