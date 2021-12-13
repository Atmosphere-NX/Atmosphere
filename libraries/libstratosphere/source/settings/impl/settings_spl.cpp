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
#include "settings_spl.hpp"

namespace ams::settings::impl {

    namespace {

        struct SplConfig {
            bool is_development;
            SplHardwareType hardware_type;
            bool is_quest;
            u64 device_id_low;
        };

        constexpr SplHardwareType ConvertToSplHardwareType(spl::HardwareType type) {
            switch (type) {
                case spl::HardwareType::Icosa:
                    return SplHardwareType_Icosa;
                case spl::HardwareType::Copper:
                    return SplHardwareType_Copper;
                case spl::HardwareType::Hoag:
                    return SplHardwareType_Hoag;
                case spl::HardwareType::Iowa:
                    return SplHardwareType_IcosaMariko;
                case spl::HardwareType::Calcio:
                    return SplHardwareType_Calcio;
                case spl::HardwareType::Aula:
                    return SplHardwareType_Aula;
                AMS_UNREACHABLE_DEFAULT_CASE();
            }
        }

        constexpr bool IsSplRetailInteractiveDisplayStateEnabled(spl::RetailInteractiveDisplayState quest_state) {
            switch (quest_state) {
                case spl::RetailInteractiveDisplayState_Disabled:
                    return false;
                case spl::RetailInteractiveDisplayState_Enabled:
                    return true;
                AMS_UNREACHABLE_DEFAULT_CASE();
            }
        }

        SplConfig GetSplConfig() {
            class SplConfigHolder {
                NON_COPYABLE(SplConfigHolder);
                NON_MOVEABLE(SplConfigHolder);
                private:
                    SplConfig m_config;
                public:
                    SplConfigHolder() {
                        /* Initialize spl. */
                        spl::Initialize();
                        ON_SCOPE_EXIT { spl::Finalize(); };

                        /* Create the config. */
                        m_config = {
                            .is_development = spl::IsDevelopment(),
                            .hardware_type  = ConvertToSplHardwareType(spl::GetHardwareType()),
                            .is_quest       = IsSplRetailInteractiveDisplayStateEnabled(spl::GetRetailInteractiveDisplayState()),
                            .device_id_low  = spl::GetDeviceIdLow(),
                        };
                    }

                    ALWAYS_INLINE operator SplConfig() {
                        return m_config;
                    }
            };

            AMS_FUNCTION_LOCAL_STATIC(SplConfigHolder, s_config_holder);
            return s_config_holder;
        }

    }

    bool IsSplDevelopment() {
        return GetSplConfig().is_development;
    }

    SplHardwareType GetSplHardwareType() {
        return GetSplConfig().hardware_type;
    }

    bool IsSplRetailInteractiveDisplayStateEnabled() {
        return GetSplConfig().is_quest;
    }

    u64 GetSplDeviceIdLow() {
        return GetSplConfig().device_id_low;
    }

}
