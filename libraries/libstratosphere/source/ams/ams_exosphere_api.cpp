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
#include <stratosphere.hpp>

namespace ams::exosphere {

    ApiInfo GetApiInfo() {
        u64 exosphere_cfg;
        if (R_FAILED(spl::impl::GetConfig(std::addressof(exosphere_cfg), spl::ConfigItem::ExosphereApiVersion))) {
            R_ABORT_UNLESS(exosphere::ResultNotPresent());
        }

        return ApiInfo{ util::BitPack64{exosphere_cfg} };
    }

    #if defined(ATMOSPHERE_BOARD_NINTENDO_NX)
    void ForceRebootToRcm() {
        R_ABORT_UNLESS(spl::impl::SetConfig(spl::ConfigItem::ExosphereNeedsReboot, 1));
    }

    void ForceRebootToIramPayload() {
        R_ABORT_UNLESS(spl::impl::SetConfig(spl::ConfigItem::ExosphereNeedsReboot, 2));
    }

    void ForceRebootToFatalError() {
        R_ABORT_UNLESS(spl::impl::SetConfig(spl::ConfigItem::ExosphereNeedsReboot, 3));
    }

    void ForceRebootByPmic() {
        R_ABORT_UNLESS(spl::impl::SetConfig(spl::ConfigItem::ExosphereNeedsReboot, 4));
    }

    void ForceShutdown() {
        R_ABORT_UNLESS(spl::impl::SetConfig(spl::ConfigItem::ExosphereNeedsShutdown, 1));
    }

    void CopyToIram(uintptr_t iram_dst, const void *dram_src, size_t size) {
        spl::smc::AtmosphereCopyToIram(iram_dst, dram_src, size);
    }

    void CopyFromIram(void *dram_dst, uintptr_t iram_src, size_t size) {
        spl::smc::AtmosphereCopyFromIram(dram_dst, iram_src, size);
    }

    namespace {

        inline u64 GetU64ConfigItem(spl::ConfigItem cfg) {
            u64 tmp;
            R_ABORT_UNLESS(spl::smc::ConvertResult(spl::smc::GetConfig(std::addressof(tmp), 1, cfg)));
            return tmp;
        }

        inline bool GetBooleanConfigItem(spl::ConfigItem cfg) {
            return GetU64ConfigItem(cfg) != 0;
        }

    }

    bool IsRcmBugPatched() {
        return spl::impl::GetConfigBool(spl::ConfigItem::ExosphereHasRcmBugPatch);
    }

    bool ShouldBlankProdInfo() {
        return spl::impl::GetConfigBool(spl::ConfigItem::ExosphereBlankProdInfo);
    }

    bool ShouldAllowWritesToProdInfo() {
        return spl::impl::GetConfigBool(spl::ConfigItem::ExosphereAllowCalWrites);
    }

    u64 GetDeviceId() {
        u64 device_id;
        R_ABORT_UNLESS(spl::impl::GetConfig(std::addressof(device_id), spl::ConfigItem::DeviceId));
        return device_id;
    }
    #endif

}
