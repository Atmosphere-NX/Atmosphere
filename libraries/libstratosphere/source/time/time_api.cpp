/*
 * Copyright (c) 2019-2020 Atmosphère-NX
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

extern "C" {

    extern TimeServiceType __nx_time_service_type;

}

namespace ams::time {

    namespace {

        enum InitializeMode {
            InitializeMode_None,
            InitializeMode_Normal,
            InitializeMode_Menu,
            InitializeMode_System,
            InitializeMode_Repair,
            InitializeMode_SystemUser,
        };

        u32 g_initialize_count = 0;
        InitializeMode g_initialize_mode = InitializeMode_None;

        /* TODO: os::SdkMutex */
        os::Mutex g_initialize_mutex(false);

        Result InitializeImpl(InitializeMode mode) {
            std::scoped_lock lk(g_initialize_mutex);

            if (g_initialize_count > 0) {
                AMS_ABORT_UNLESS(mode == g_initialize_mode);
                g_initialize_count++;
                return ResultSuccess();
            }

            switch (mode) {
                case InitializeMode_Normal:     __nx_time_service_type = ::TimeServiceType_User;       break;
                case InitializeMode_Menu:       __nx_time_service_type = ::TimeServiceType_Menu;       break;
                case InitializeMode_System:     __nx_time_service_type = ::TimeServiceType_System;     break;
                case InitializeMode_Repair:     __nx_time_service_type = ::TimeServiceType_Repair;     break;
                case InitializeMode_SystemUser: __nx_time_service_type = ::TimeServiceType_SystemUser; break;
                AMS_UNREACHABLE_DEFAULT_CASE();
            }

            R_TRY(::timeInitialize());

            g_initialize_count++;
            g_initialize_mode = mode;
            return ResultSuccess();
        }

    }

    Result Initialize() {
        return InitializeImpl(InitializeMode_Normal);
    }

    Result InitializeForSystem() {
        return InitializeImpl(InitializeMode_System);
    }

    Result InitializeForSystemUser() {
        return InitializeImpl(InitializeMode_System);
    }

    Result Finalize() {
        std::scoped_lock lk(g_initialize_mutex);

        if (g_initialize_count > 0) {
            if ((--g_initialize_count) == 0) {
                ::timeExit();
                g_initialize_mode = InitializeMode_None;
            }
        }

        return ResultSuccess();
    }

    bool IsInitialized() {
        std::scoped_lock lk(g_initialize_mutex);

        return g_initialize_count > 0;
    }

    Result GetElapsedSecondsBetween(s64 *out, const SteadyClockTimePoint &from, const SteadyClockTimePoint &to) {
        return impl::util::GetSpanBetween(out, from, to);
    }

}
