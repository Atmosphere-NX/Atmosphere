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
        os::Mutex g_mutex(false);

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
            AMS_ABORT_UNLESS(g_hos_version <= hos::Version_Max);

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
