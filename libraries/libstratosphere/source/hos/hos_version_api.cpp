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
#include "hos_version_api_private.hpp"

namespace ams::hos {

    namespace {

        hos::Version g_hos_version;
        bool g_has_cached;
        os::SdkMutex g_mutex;

        void CacheValues() {
            if (__atomic_load_n(&g_has_cached, __ATOMIC_SEQ_CST)) {
                return;
            }

            std::scoped_lock lk(g_mutex);

            if (g_has_cached) {
                return;
            }

            /* Hos version is a direct copy of target firmware, just renamed. */
            g_hos_version = static_cast<hos::Version>(exosphere::GetApiInfo().GetTargetFirmware());

            /* Ensure that this is a hos version we can sanely *try* to run. */
            /* To be friendly, we will only require that we recognize the major and minor versions. */
            /* We can consider only recognizing major in the future, but micro seems safe to ignore as */
            /* there are no breaking IPC changes in minor updates. */
            {
                constexpr u32 MaxMajor = (static_cast<u32>(hos::Version_Max) >> 24) & 0xFF;
                constexpr u32 MaxMinor = (static_cast<u32>(hos::Version_Max) >> 16) & 0xFF;

                const u32 major = (static_cast<u32>(g_hos_version) >> 24) & 0xFF;
                const u32 minor = (static_cast<u32>(g_hos_version) >> 16) & 0xFF;

                const bool is_safely_tryable_version = (g_hos_version <= hos::Version_Max) || (major == MaxMajor && minor <= MaxMinor);
                AMS_ABORT_UNLESS(is_safely_tryable_version);
            }

            __atomic_store_n(&g_has_cached, true, __ATOMIC_SEQ_CST);
        }

    }

    ::ams::hos::Version GetVersion() {
        CacheValues();
        return g_hos_version;
    }


    void SetVersionForLibnxInternalDebug(hos::Version debug_version) {
        std::scoped_lock lk(g_mutex);
        g_hos_version = debug_version;
        __atomic_store_n(&g_has_cached, true, __ATOMIC_SEQ_CST);
        SetVersionForLibnxInternal();
    }

    void SetVersionForLibnxInternal() {
        const u32 hos_version_val = static_cast<u32>(hos::GetVersion());
        const u32 major = (hos_version_val >> 24) & 0xFF;
        const u32 minor = (hos_version_val >> 16) & 0xFF;
        const u32 micro = (hos_version_val >>  8) & 0xFF;
        hosversionSet(MAKEHOSVERSION(major, minor, micro));
    }

}
