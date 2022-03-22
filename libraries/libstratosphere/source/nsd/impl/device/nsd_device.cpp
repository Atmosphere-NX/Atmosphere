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

namespace ams::nsd::impl::device {

    namespace {

        constexpr const char SettingsName[] = "nsd";
        constexpr const char SettingsKey[]  = "environment_identifier";

        constinit os::SdkMutex g_environment_identifier_lock;
        constinit EnvironmentIdentifier g_environment_identifier = {};
        constinit bool g_determined_environment_identifier = false;

    }

    const EnvironmentIdentifier &GetEnvironmentIdentifierFromSettings() {
        if (AMS_UNLIKELY(!g_determined_environment_identifier)) {
            std::scoped_lock lk(g_environment_identifier_lock);

            if (AMS_LIKELY(!g_determined_environment_identifier)) {
                /* Check size. */
                AMS_ABORT_UNLESS(settings::fwdbg::GetSettingsItemValueSize(SettingsName, SettingsKey) != 0);

                /* Get value. */
                const size_t size = settings::fwdbg::GetSettingsItemValue(g_environment_identifier.value, EnvironmentIdentifier::Size, SettingsName, SettingsKey);
                AMS_ABORT_UNLESS(size < EnvironmentIdentifier::Size);
                AMS_ABORT_UNLESS(g_environment_identifier == EnvironmentIdentifierOfProductDevice || g_environment_identifier == EnvironmentIdentifierOfNotProductDevice);

                g_determined_environment_identifier = true;
            }
        }

        return g_environment_identifier;
    }

}
