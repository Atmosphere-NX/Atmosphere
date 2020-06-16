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

namespace ams::cfg {

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
        os::Mutex g_sd_card_lock(false);
        bool g_sd_card_initialized = false;
        FsFileSystem g_sd_card_filesystem = {};

        /* SD card helpers. */
        Result CheckSdCardServicesReady() {
            for (size_t i = 0; i < NumRequiredServicesForSdCardAccess; i++) {
                bool service_present = false;
                R_TRY(sm::HasService(&service_present, RequiredServicesForSdCardAccess[i]));
                if (!service_present) {
                    return fs::ResultSdCardNotPresent();
                }
            }

            return ResultSuccess();
        }

        void WaitSdCardServicesReadyImpl() {
            for (size_t i = 0; i < NumRequiredServicesForSdCardAccess; i++) {
                R_ABORT_UNLESS(sm::WaitService(RequiredServicesForSdCardAccess[i]));
            }
        }

        Result TryInitializeSdCard() {
            R_TRY(CheckSdCardServicesReady());
            R_ABORT_UNLESS(fsOpenSdCardFileSystem(&g_sd_card_filesystem));
            g_sd_card_initialized = true;
            return ResultSuccess();
        }

        void InitializeSdCard() {
            WaitSdCardServicesReadyImpl();
            R_ABORT_UNLESS(fsOpenSdCardFileSystem(&g_sd_card_filesystem));
            g_sd_card_initialized = true;
        }

    }

    /* SD card utilities. */
    bool IsSdCardRequiredServicesReady() {
        return R_SUCCEEDED(CheckSdCardServicesReady());
    }

    void WaitSdCardRequiredServicesReady() {
        WaitSdCardServicesReadyImpl();
    }

    bool IsSdCardInitialized() {
        std::scoped_lock lk(g_sd_card_lock);

        if (!g_sd_card_initialized) {
            if (R_SUCCEEDED(TryInitializeSdCard())) {
                g_sd_card_initialized = true;
            }
        }
        return g_sd_card_initialized;
    }

    void WaitSdCardInitialized() {
        std::scoped_lock lk(g_sd_card_lock);

        InitializeSdCard();
    }

}
