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
#include "pm_resource_manager.hpp"

namespace ams::pm::resource {

    namespace {

        constexpr LimitableResource LimitableResources[] = {
            LimitableResource_Memory,
            LimitableResource_Threads,
            LimitableResource_Events,
            LimitableResource_TransferMemories,
            LimitableResource_Sessions,
        };
        constexpr size_t LimitableResource_Count = util::size(LimitableResources);

        constexpr size_t Megabyte = 0x100000;

        /* Definitions for limit differences over time. */
        constexpr size_t ExtraSystemThreadCount400   = 100;
        constexpr size_t ExtraSystemMemorySize400    = 10 * Megabyte;
        constexpr size_t ExtraSystemMemorySize500    = 12 * Megabyte;
        constexpr size_t ExtraSystemEventCount600    = 100;
        constexpr size_t ExtraSystemSessionCount600  = 100;
        constexpr size_t ReservedMemorySize600       = 5 * Megabyte;

        /* Atmosphere always allocates 24 extra megabytes for system usage. */
        constexpr size_t ExtraSystemMemorySizeAtmosphere = 24 * Megabyte;

        /* Globals. */
        os::Mutex g_resource_limit_lock;
        Handle g_resource_limit_handles[ResourceLimitGroup_Count];
        spl::MemoryArrangement g_memory_arrangement = spl::MemoryArrangement_Standard;
        u64 g_system_memory_boost_size = 0;
        u64 g_extra_application_threads_available = 0;

        u64 g_resource_limits[ResourceLimitGroup_Count][LimitableResource_Count] = {
            [ResourceLimitGroup_System] = {
                [LimitableResource_Memory]           = 0,   /* Initialized by more complicated logic later. */
                [LimitableResource_Threads]          = 508,
                [LimitableResource_Events]           = 600,
                [LimitableResource_TransferMemories] = 128,
                [LimitableResource_Sessions]         = 794,
            },
            [ResourceLimitGroup_Application] = {
                [LimitableResource_Memory]           = 0,   /* Initialized by more complicated logic later. */
                [LimitableResource_Threads]          = 96,
                [LimitableResource_Events]           = 0,
                [LimitableResource_TransferMemories] = 32,
                [LimitableResource_Sessions]         = 1,
            },
            [ResourceLimitGroup_Applet] = {
                [LimitableResource_Memory]           = 0,   /* Initialized by more complicated logic later. */
                [LimitableResource_Threads]          = 96,
                [LimitableResource_Events]           = 0,
                [LimitableResource_TransferMemories] = 32,
                [LimitableResource_Sessions]         = 5,
            },
        };

        u64 g_memory_resource_limits[spl::MemoryArrangement_Count][ResourceLimitGroup_Count] = {
            [spl::MemoryArrangement_Standard] = {
                [ResourceLimitGroup_System]      = 269  * Megabyte,
                [ResourceLimitGroup_Application] = 3285 * Megabyte,
                [ResourceLimitGroup_Applet]      = 535  * Megabyte,
            },
            [spl::MemoryArrangement_StandardForAppletDev] = {
                [ResourceLimitGroup_System]      = 481  * Megabyte,
                [ResourceLimitGroup_Application] = 2048 * Megabyte,
                [ResourceLimitGroup_Applet]      = 1560 * Megabyte,
            },
            [spl::MemoryArrangement_StandardForSystemDev] = {
                [ResourceLimitGroup_System]      = 328  * Megabyte,
                [ResourceLimitGroup_Application] = 3285 * Megabyte,
                [ResourceLimitGroup_Applet]      = 476  * Megabyte,
            },
            [spl::MemoryArrangement_Expanded] = {
                [ResourceLimitGroup_System]      = 653  * Megabyte,
                [ResourceLimitGroup_Application] = 4916 * Megabyte,
                [ResourceLimitGroup_Applet]      = 568  * Megabyte,
            },
            [spl::MemoryArrangement_ExpandedForAppletDev] = {
                [ResourceLimitGroup_System]      = 653  * Megabyte,
                [ResourceLimitGroup_Application] = 3285 * Megabyte,
                [ResourceLimitGroup_Applet]      = 2199 * Megabyte,
            },
        };

        /* Helpers. */
        Result SetMemoryResourceLimitLimitValue(ResourceLimitGroup group, u64 new_memory_limit) {
            const u64 old_memory_limit = g_resource_limits[group][LimitableResource_Memory];
            g_resource_limits[group][LimitableResource_Memory] = new_memory_limit;

            {
                /* If we fail, restore the old memory limit. */
                auto limit_guard = SCOPE_GUARD { g_resource_limits[group][LimitableResource_Memory] = old_memory_limit; };
                R_TRY(svcSetResourceLimitLimitValue(GetResourceLimitHandle(group), LimitableResource_Memory, g_resource_limits[group][LimitableResource_Memory]));
                limit_guard.Cancel();
            }

            return ResultSuccess();
        }

        Result SetResourceLimitLimitValues(ResourceLimitGroup group, u64 new_memory_limit) {
            /* First, set memory limit. */
            R_TRY(SetMemoryResourceLimitLimitValue(group, new_memory_limit));

            /* Set other limit values. */
            for (size_t i = 0; i < LimitableResource_Count; i++) {
                const auto resource = LimitableResources[i];
                if (resource == LimitableResource_Memory) {
                    continue;
                }
                R_TRY(svcSetResourceLimitLimitValue(GetResourceLimitHandle(group), resource, g_resource_limits[group][resource]));
            }
            return ResultSuccess();
        }

        inline ResourceLimitGroup GetResourceLimitGroup(const ldr::ProgramInfo *info) {
            switch (info->flags & ldr::ProgramInfoFlag_ApplicationTypeMask) {
                case ldr::ProgramInfoFlag_Application:
                    return ResourceLimitGroup_Application;
                case ldr::ProgramInfoFlag_Applet:
                    return ResourceLimitGroup_Applet;
                default:
                    return ResourceLimitGroup_System;
            }
        }

        void WaitResourceAvailable(ResourceLimitGroup group) {
            const Handle reslimit_hnd = GetResourceLimitHandle(group);
            for (size_t i = 0; i < LimitableResource_Count; i++) {
                const auto resource = LimitableResources[i];

                u64 value = 0;
                while (true) {
                    R_ASSERT(svcGetResourceLimitCurrentValue(&value, reslimit_hnd, resource));
                    if (value == 0) {
                        break;
                    }
                    svcSleepThread(1'000'000ul);
                }
            }
        }

        void WaitApplicationMemoryAvailable() {
            u64 value = 0;
            while (true) {
                R_ASSERT(svcGetSystemInfo(&value, SystemInfoType_UsedPhysicalMemorySize, INVALID_HANDLE, PhysicalMemoryInfo_Application));
                if (value == 0) {
                    break;
                }
                svcSleepThread(1'000'000ul);
            }
        }

    }

    /* Resource API. */
    Result InitializeResourceManager() {
        /* Create resource limit handles. */
        for (size_t i = 0; i < ResourceLimitGroup_Count; i++) {
            if (i == ResourceLimitGroup_System) {
                u64 value = 0;
                R_ASSERT(svcGetInfo(&value, InfoType_ResourceLimit, INVALID_HANDLE, 0));
                g_resource_limit_handles[i] = static_cast<Handle>(value);
            } else {
                R_ASSERT(svcCreateResourceLimit(&g_resource_limit_handles[i]));
            }
        }

        /* Adjust resource limits based on hos firmware version. */
        const auto hos_version = hos::GetVersion();
        if (hos_version >= hos::Version_400) {
            /* 4.0.0 increased the system thread limit. */
            g_resource_limits[ResourceLimitGroup_System][LimitableResource_Threads] += ExtraSystemThreadCount400;
            /* 4.0.0 also took memory away from applet and gave it to system, for the Standard and StandardForSystemDev profiles. */
            g_memory_resource_limits[spl::MemoryArrangement_Standard][ResourceLimitGroup_System] += ExtraSystemMemorySize400;
            g_memory_resource_limits[spl::MemoryArrangement_Standard][ResourceLimitGroup_Applet] -= ExtraSystemMemorySize400;
            g_memory_resource_limits[spl::MemoryArrangement_StandardForSystemDev][ResourceLimitGroup_System] += ExtraSystemMemorySize400;
            g_memory_resource_limits[spl::MemoryArrangement_StandardForSystemDev][ResourceLimitGroup_Applet] -= ExtraSystemMemorySize400;
        }
        if (hos_version >= hos::Version_500) {
            /* 5.0.0 took more memory away from applet and gave it to system, for the Standard and StandardForSystemDev profiles. */
            g_memory_resource_limits[spl::MemoryArrangement_Standard][ResourceLimitGroup_System] += ExtraSystemMemorySize500;
            g_memory_resource_limits[spl::MemoryArrangement_Standard][ResourceLimitGroup_Applet] -= ExtraSystemMemorySize500;
            g_memory_resource_limits[spl::MemoryArrangement_StandardForSystemDev][ResourceLimitGroup_System] += ExtraSystemMemorySize500;
            g_memory_resource_limits[spl::MemoryArrangement_StandardForSystemDev][ResourceLimitGroup_Applet] -= ExtraSystemMemorySize500;
        }
        if (hos_version >= hos::Version_600) {
            /* 6.0.0 increased the system event and session limits. */
            g_resource_limits[ResourceLimitGroup_System][LimitableResource_Events]   += ExtraSystemEventCount600;
            g_resource_limits[ResourceLimitGroup_System][LimitableResource_Sessions] += ExtraSystemSessionCount600;
        }

        /* 7.0.0+: Calculate the number of extra application threads available. */
        if (hos::GetVersion() >= hos::Version_700) {
            /* See how many threads we have available. */
            u64 total_threads_available = 0;
            R_ASSERT(svcGetResourceLimitLimitValue(&total_threads_available, GetResourceLimitHandle(ResourceLimitGroup_System), LimitableResource_Threads));

            /* See how many threads we're expecting. */
            const size_t total_threads_allocated = g_resource_limits[ResourceLimitGroup_System][LimitableResource_Threads] -
                                                       g_resource_limits[ResourceLimitGroup_Application][LimitableResource_Threads] -
                                                       g_resource_limits[ResourceLimitGroup_Applet][LimitableResource_Threads];

            /* Ensure we don't over-commit threads. */
            AMS_ASSERT(total_threads_allocated <= total_threads_available);

            /* Set number of extra threads. */
            g_extra_application_threads_available = total_threads_available - total_threads_allocated;
        }

        /* Choose and initialize memory arrangement. */
        if (hos_version >= hos::Version_600) {
            /* 6.0.0 retrieves memory limit information from the kernel, rather than using a hardcoded profile. */
            g_memory_arrangement = spl::MemoryArrangement_Dynamic;

            /* Get total memory available. */
            u64 total_memory = 0;
            R_ASSERT(svcGetResourceLimitLimitValue(&total_memory, GetResourceLimitHandle(ResourceLimitGroup_System), LimitableResource_Memory));

            /* Get and save application + applet memory. */
            R_ASSERT(svcGetSystemInfo(&g_memory_resource_limits[spl::MemoryArrangement_Dynamic][ResourceLimitGroup_Application], SystemInfoType_TotalPhysicalMemorySize, INVALID_HANDLE, PhysicalMemoryInfo_Application));
            R_ASSERT(svcGetSystemInfo(&g_memory_resource_limits[spl::MemoryArrangement_Dynamic][ResourceLimitGroup_Applet],      SystemInfoType_TotalPhysicalMemorySize, INVALID_HANDLE, PhysicalMemoryInfo_Applet));

            const u64 application_size         = g_memory_resource_limits[spl::MemoryArrangement_Dynamic][ResourceLimitGroup_Application];
            const u64 applet_size              = g_memory_resource_limits[spl::MemoryArrangement_Dynamic][ResourceLimitGroup_Applet];
            const u64 reserved_non_system_size = (application_size + applet_size + ReservedMemorySize600);

            /* Ensure there's enough memory for the system region. */
            AMS_ASSERT(reserved_non_system_size < total_memory);

            g_memory_resource_limits[spl::MemoryArrangement_Dynamic][ResourceLimitGroup_System] = total_memory - reserved_non_system_size;
        } else {
            g_memory_arrangement = spl::GetMemoryArrangement();
        }

        /* Adjust memory limits for atmosphere. */
        /* We take memory away from applet normally, but away from application on < 3.0.0 to avoid a rare hang on boot. */
        for (size_t i = 0; i < spl::MemoryArrangement_Count; i++) {
            g_memory_resource_limits[i][ResourceLimitGroup_System] += ExtraSystemMemorySizeAtmosphere;
            if (hos_version >= hos::Version_300) {
                g_memory_resource_limits[i][ResourceLimitGroup_Applet] -= ExtraSystemMemorySizeAtmosphere;
            } else {
                g_memory_resource_limits[i][ResourceLimitGroup_Application] -= ExtraSystemMemorySizeAtmosphere;
            }
        }

        /* Actually set resource limits. */
        {
            std::scoped_lock lk(g_resource_limit_lock);

            for (size_t group = 0; group < ResourceLimitGroup_Count; group++) {
                R_ASSERT(SetResourceLimitLimitValues(static_cast<ResourceLimitGroup>(group), g_memory_resource_limits[g_memory_arrangement][group]));
            }
        }

        return ResultSuccess();
    }

    Result BoostSystemMemoryResourceLimit(u64 boost_size) {
        /* Don't allow all application memory to be taken away. */
        R_UNLESS(boost_size <= g_memory_resource_limits[g_memory_arrangement][ResourceLimitGroup_Application], pm::ResultInvalidSize());

        const u64 new_app_size = g_memory_resource_limits[g_memory_arrangement][ResourceLimitGroup_Application] - boost_size;
        {
            std::scoped_lock lk(g_resource_limit_lock);

            if (hos::GetVersion() >= hos::Version_500) {
                /* Starting in 5.0.0, PM does not allow for only one of the sets to fail. */
                if (boost_size < g_system_memory_boost_size) {
                    R_TRY(svcSetUnsafeLimit(boost_size));
                    R_ASSERT(SetMemoryResourceLimitLimitValue(ResourceLimitGroup_Application, new_app_size));
                } else {
                    R_TRY(SetMemoryResourceLimitLimitValue(ResourceLimitGroup_Application, new_app_size));
                    R_ASSERT(svcSetUnsafeLimit(boost_size));
                }
            } else {
                const u64 new_sys_size = g_memory_resource_limits[g_memory_arrangement][ResourceLimitGroup_System] + boost_size;
                if (boost_size < g_system_memory_boost_size) {
                    R_TRY(SetMemoryResourceLimitLimitValue(ResourceLimitGroup_System,      new_sys_size));
                    R_TRY(SetMemoryResourceLimitLimitValue(ResourceLimitGroup_Application, new_app_size));
                } else {
                    R_TRY(SetMemoryResourceLimitLimitValue(ResourceLimitGroup_Application, new_app_size));
                    R_TRY(SetMemoryResourceLimitLimitValue(ResourceLimitGroup_System,      new_sys_size));
                }
            }

            g_system_memory_boost_size = boost_size;
        }

        return ResultSuccess();
    }

    Result BoostApplicationThreadResourceLimit() {
        std::scoped_lock lk(g_resource_limit_lock);
        /* Set new limit. */
        const u64 new_thread_count = g_resource_limits[ResourceLimitGroup_Application][LimitableResource_Threads] + g_extra_application_threads_available;
        R_TRY(svcSetResourceLimitLimitValue(GetResourceLimitHandle(ResourceLimitGroup_Application), LimitableResource_Threads, new_thread_count));

        /* Record that we did so. */
        g_resource_limits[ResourceLimitGroup_Application][LimitableResource_Threads] = new_thread_count;
        g_extra_application_threads_available = 0;

        return ResultSuccess();
    }

    Handle GetResourceLimitHandle(ResourceLimitGroup group) {
        return g_resource_limit_handles[group];
    }

    Handle GetResourceLimitHandle(const ldr::ProgramInfo *info) {
        return GetResourceLimitHandle(GetResourceLimitGroup(info));
    }

    void WaitResourceAvailable(const ldr::ProgramInfo *info) {
        if (GetResourceLimitGroup(info) == ResourceLimitGroup_Application) {
            WaitResourceAvailable(ResourceLimitGroup_Application);
            if (hos::GetVersion() >= hos::Version_500) {
                WaitApplicationMemoryAvailable();
            }
        }
    }

    Result GetResourceLimitValues(u64 *out_cur, u64 *out_lim, ResourceLimitGroup group, LimitableResource resource) {
        /* Do not allow out of bounds access. */
        AMS_ASSERT(group < ResourceLimitGroup_Count);
        AMS_ASSERT(resource < LimitableResource_Count);

        const Handle reslimit_hnd = GetResourceLimitHandle(group);
        R_TRY(svcGetResourceLimitCurrentValue(out_cur, reslimit_hnd, resource));
        R_TRY(svcGetResourceLimitLimitValue(out_lim, reslimit_hnd, resource));

        return ResultSuccess();
    }

}
