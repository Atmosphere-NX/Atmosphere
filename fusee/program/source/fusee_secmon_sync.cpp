/*
 * Copyright (c) Atmosphère-NX
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
#include "fusee_secmon_sync.hpp"

namespace ams::nxboot {

    namespace {

        ALWAYS_INLINE pkg1::SecureMonitorParameters &GetSecureMonitorParameters() {
            return *secmon::MemoryRegionPhysicalDeviceBootloaderParams.GetPointer<pkg1::SecureMonitorParameters>();
        }

    }

    void InitializeSecureMonitorMailbox() {
        std::memset(std::addressof(GetSecureMonitorParameters()), 0, sizeof(GetSecureMonitorParameters()));
    }

    void WaitSecureMonitorState(pkg1::SecureMonitorState state) {
        auto &secmon_params = GetSecureMonitorParameters();

        while (secmon_params.secmon_state != state) {
            hw::InvalidateDataCache(std::addressof(secmon_params.secmon_state), sizeof(secmon_params.secmon_state));
            util::WaitMicroSeconds(1);
        }
    }

    void SetBootloaderState(pkg1::BootloaderState state) {
        auto &secmon_params = GetSecureMonitorParameters();
        secmon_params.bootloader_state = state;
        hw::FlushDataCache(std::addressof(secmon_params.bootloader_state), sizeof(secmon_params.bootloader_state));
    }

}