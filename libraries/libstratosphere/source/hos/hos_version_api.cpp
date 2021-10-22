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

namespace ams::hos {

    namespace {

        constinit os::SdkMutex g_hos_init_lock;
        constinit hos::Version g_hos_version;
        constinit bool g_set_hos_version;

        Result GetExosphereApiInfo(exosphere::ApiInfo *out) {
            u64 exosphere_cfg;
            R_TRY_CATCH(spl::impl::GetConfig(std::addressof(exosphere_cfg), spl::ConfigItem::ExosphereApiVersion)) {
                R_CATCH_RETHROW(spl::ResultSecureMonitorNotInitialized)
            } R_END_TRY_CATCH_WITH_ABORT_UNLESS;

            *out = { exosphere_cfg };
            return ResultSuccess();
        }

        Result GetApproximateExosphereApiInfo(exosphere::ApiInfo *out) {
            u64 exosphere_cfg;

            R_TRY_CATCH(spl::impl::GetConfig(std::addressof(exosphere_cfg), spl::ConfigItem::ExosphereApproximateApiVersion)) {
                R_CATCH_RETHROW(spl::ResultSecureMonitorBusy)
            } R_END_TRY_CATCH_WITH_ABORT_UNLESS;

            *out = { exosphere_cfg };
            return ResultSuccess();
        }

        exosphere::ApiInfo GetExosphereApiInfo(bool allow_approximate) {
            exosphere::ApiInfo info;
            while (true) {
                if (R_SUCCEEDED(GetExosphereApiInfo(std::addressof(info)))) {
                    return info;
                }

                if (allow_approximate && R_SUCCEEDED(GetApproximateExosphereApiInfo(std::addressof(info)))) {
                    return info;
                }

                svc::SleepThread(TimeSpan::FromMilliSeconds(25).GetNanoSeconds());
            }
        }

    }

    bool IsUnitTestProgramForSetVersion();

    void InitializeVersionInternal(bool allow_approximate) {
        hos::Version current = hos::Version_Current;

        /* If we're unit testing, just set the version and move on. */
        if (IsUnitTestProgramForSetVersion()) {
            g_hos_version     = hos::Version_Current;
            g_set_hos_version = true;
        } else {
            /* Get the current (and previous approximation of) target firmware. */
            hos::Version prev;
            bool has_prev = false;
            {
                /* Acquire exclusive access to set hos version. */
                std::scoped_lock lk(g_hos_init_lock);

                /* Save the previous value of g_hos_version. */
                prev     = g_hos_version;
                has_prev = g_set_hos_version;

                /* Set hos version = exosphere api version target firmware. */
                g_hos_version = static_cast<hos::Version>(GetExosphereApiInfo(allow_approximate).GetTargetFirmware());

                /* Save the current value of g_hos_version. */
                current = g_hos_version;

                /* Note that we've set a previous hos version. */
                g_set_hos_version = true;
            }

            /* Ensure that this is a hos version we can sanely *try* to run. */
            /* To be friendly, we will only require that we recognize the major and minor versions. */
            /* We can consider only recognizing major in the future, but micro seems safe to ignore as */
            /* there are no breaking IPC changes in minor updates. */
            {
                constexpr u32 MaxMajor = (static_cast<u32>(hos::Version_Max) >> 24) & 0xFF;
                constexpr u32 MaxMinor = (static_cast<u32>(hos::Version_Max) >> 16) & 0xFF;

                const u32 major = (static_cast<u32>(current) >> 24) & 0xFF;
                const u32 minor = (static_cast<u32>(current) >> 16) & 0xFF;

                const bool is_safely_tryable_version = (current <= hos::Version_Max) || (major == MaxMajor && minor <= MaxMinor);
                AMS_ABORT_UNLESS(is_safely_tryable_version);
            }

            /* Ensure that this is a hos version compatible with previous approximations. */
            if (has_prev) {
                AMS_ABORT_UNLESS(current >= prev);

                const u32 current_major = (static_cast<u32>(current) >> 24) & 0xFF;
                const u32 prev_major    = (static_cast<u32>(prev) >> 24) & 0xFF;

                AMS_ABORT_UNLESS(current_major == prev_major);
            }
        }

        /* Set the version for libnx. */
        {
            const u32 major = (static_cast<u32>(current) >> 24) & 0xFF;
            const u32 minor = (static_cast<u32>(current) >> 16) & 0xFF;
            const u32 micro = (static_cast<u32>(current) >>  8) & 0xFF;
            hosversionSet((BIT(31)) | (MAKEHOSVERSION(major, minor, micro)));
        }
    }

    ::ams::hos::Version GetVersion() {
        return g_hos_version;
    }

}
