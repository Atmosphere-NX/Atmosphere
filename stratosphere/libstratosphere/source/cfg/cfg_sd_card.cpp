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

#include <switch.h>
#include <stratosphere.hpp>
#include <stratosphere/cfg.hpp>
#include <stratosphere/sm.hpp>

namespace sts::cfg {

    namespace {

        /* Convenience definitions. */
        constexpr sm::ServiceName RequiredServicesForSdCardAccess[] = {
            sm::ServiceName::Encode("pcv"),
            sm::ServiceName::Encode("gpio"),
            sm::ServiceName::Encode("pinmux"),
            sm::ServiceName::Encode("psc:m"),
        };
        constexpr size_t NumRequiredServicesForSdCardAccess = util::size(RequiredServicesForSdCardAccess);

        /* SD card globals. */
        HosMutex g_sd_card_lock;
        bool g_sd_card_initialized = false;
        FsFileSystem g_sd_card_filesystem = {};

        /* SD card helpers. */
        Result TryInitializeSdCard() {
            for (size_t i = 0; i < NumRequiredServicesForSdCardAccess; i++) {
                bool service_present = false;
                R_TRY(sm::HasService(&service_present, RequiredServicesForSdCardAccess[i]));
                if (!service_present) {
                    return ResultFsSdCardNotPresent;
                }
            }

            R_ASSERT(fsMountSdcard(&g_sd_card_filesystem));
            g_sd_card_initialized = true;
            return ResultSuccess;
        }

        void InitializeSdCard() {
            for (size_t i = 0; i < NumRequiredServicesForSdCardAccess; i++) {
                R_ASSERT(sm::WaitService(RequiredServicesForSdCardAccess[i]));
            }
            R_ASSERT(fsMountSdcard(&g_sd_card_filesystem));
            g_sd_card_initialized = true;
        }

    }

    /* SD card utilities. */
    bool IsSdCardInitialized() {
        std::scoped_lock<HosMutex> lk(g_sd_card_lock);

        if (!g_sd_card_initialized) {
            if (R_SUCCEEDED(TryInitializeSdCard())) {
                g_sd_card_initialized = true;
            }
        }
        return g_sd_card_initialized;
    }

    void WaitSdCardInitialized() {
        std::scoped_lock<HosMutex> lk(g_sd_card_lock);

        InitializeSdCard();
    }

}
