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

#include <stratosphere.hpp>

namespace ams::hos {

    namespace {

        hos::Version g_hos_version;
        bool g_has_cached;
        os::Mutex g_mutex;

        void CacheValues() {
            if (__atomic_load_n(&g_has_cached, __ATOMIC_SEQ_CST)) {
                return;
            }

            std::scoped_lock lk(g_mutex);

            if (g_has_cached) {
                return;
            }

            switch (exosphere::GetApiInfo().GetTargetFirmware()) {
                case exosphere::TargetFirmware_100:
                    g_hos_version = hos::Version_100;
                    break;
                case exosphere::TargetFirmware_200:
                    g_hos_version = hos::Version_200;
                    break;
                case exosphere::TargetFirmware_300:
                    g_hos_version = hos::Version_300;
                    break;
                case exosphere::TargetFirmware_400:
                    g_hos_version = hos::Version_400;
                    break;
                case exosphere::TargetFirmware_500:
                    g_hos_version = hos::Version_500;
                    break;
                case exosphere::TargetFirmware_600:
                case exosphere::TargetFirmware_620:
                    g_hos_version = hos::Version_600;
                    break;
                case exosphere::TargetFirmware_700:
                    g_hos_version = hos::Version_700;
                    break;
                case exosphere::TargetFirmware_800:
                    g_hos_version = hos::Version_800;
                    break;
                case exosphere::TargetFirmware_810:
                    g_hos_version = hos::Version_810;
                    break;
                case exosphere::TargetFirmware_900:
                    g_hos_version = hos::Version_900;
                case exosphere::TargetFirmware_910:
                    g_hos_version = hos::Version_910;
                    break;
                AMS_UNREACHABLE_DEFAULT_CASE();
            }

            __atomic_store_n(&g_has_cached, true, __ATOMIC_SEQ_CST);
        }

    }

    ::ams::hos::Version GetVersion() {
        CacheValues();
        return g_hos_version;
    }

    void SetVersionForLibnx() {
        u32 major = 0, minor = 0, micro = 0;
        switch (hos::GetVersion()) {
            case hos::Version_100:
                major = 1;
                minor = 0;
                micro = 0;
                break;
            case hos::Version_200:
                major = 2;
                minor = 0;
                micro = 0;
                break;
            case hos::Version_300:
                major = 3;
                minor = 0;
                micro = 0;
                break;
            case hos::Version_400:
                major = 4;
                minor = 0;
                micro = 0;
                break;
            case hos::Version_500:
                major = 5;
                minor = 0;
                micro = 0;
                break;
            case hos::Version_600:
                major = 6;
                minor = 0;
                micro = 0;
                break;
            case hos::Version_700:
                major = 7;
                minor = 0;
                micro = 0;
                break;
            case hos::Version_800:
                major = 8;
                minor = 0;
                micro = 0;
                break;
            case hos::Version_810:
                major = 8;
                minor = 1;
                micro = 0;
                break;
            case hos::Version_900:
                major = 9;
                minor = 0;
                micro = 0;
            case hos::Version_910:
                major = 9;
                minor = 1;
                micro = 0;
                break;
            AMS_UNREACHABLE_DEFAULT_CASE();
        }
        hosversionSet(MAKEHOSVERSION(major, minor, micro));
    }

}
