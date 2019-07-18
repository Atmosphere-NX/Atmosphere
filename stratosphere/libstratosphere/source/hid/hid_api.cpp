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
#include <set>
#include <switch.h>
#include <stratosphere.hpp>
#include <stratosphere/pm.hpp>
#include <stratosphere/hid.hpp>

namespace sts::hid {

    namespace {

        /* Global lock. */
        HosMutex g_hid_lock;
        bool g_initialized_hid = false;

        /* Helper. */
        void InitializeHid() {
            R_ASSERT(smInitialize());
            ON_SCOPE_EXIT { smExit(); };
            {
                R_ASSERT(hidInitialize());
            }
        }

        Result EnsureHidInitialized() {
            if (!g_initialized_hid) {
                if (!serviceIsActive(hidGetServiceSession())) {
                    if (!pm::info::HasLaunchedTitle(ncm::TitleId::Hid)) {
                        return MAKERESULT(Module_Libnx, LibnxError_InitFail_HID);
                    }
                    InitializeHid();
                }
                g_initialized_hid = true;
            }

            return ResultSuccess;
        }

    }

    Result GetKeysHeld(u64 *out) {
        std::scoped_lock<HosMutex> lk(g_hid_lock);

        R_TRY(EnsureHidInitialized());

        hidScanInput();
        *out = 0;

        for (size_t controller = 0; controller < 10; controller++) {
            *out |= hidKeysHeld(static_cast<HidControllerID>(controller));
        }

        return ResultSuccess;
    }

}
