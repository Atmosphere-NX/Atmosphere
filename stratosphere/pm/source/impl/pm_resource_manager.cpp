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
#include "pm_resource_manager.hpp"

namespace ams::pm::resource {

    namespace {

        constexpr svc::LimitableResource LimitableResources[] = {
            svc::LimitableResource_PhysicalMemoryMax,
            svc::LimitableResource_ThreadCountMax,
            svc::LimitableResource_EventCountMax,
            svc::LimitableResource_TransferMemoryCountMax,
            svc::LimitableResource_SessionCountMax,
        };

        /* Definitions for limit differences over time. */
        constexpr size_t ExtraSystemThreadCount400   = 100;
        constexpr size_t ExtraSystemMemorySize400    = 10_MB;
        constexpr size_t ExtraSystemMemorySize500    = 12_MB;
        constexpr size_t ExtraSystemEventCount600    = 100;
        constexpr size_t ExtraSystemSessionCount600  = 100;
        constexpr size_t ReservedMemorySize600       = 5_MB;
        constexpr size_t ExtraSystemSessionCount920  = 33;

        /* Atmosphere always allocates extra memory for system usage. */
        constexpr size_t ExtraSystemMemorySizeAtmosphere    = 24_MB;
        constexpr size_t ExtraSystemMemorySizeAtmosphere500 = 33_MB; /* Applet pool is 0x20100000 */

        /* Globals. */
        os::Mutex g_resource_limit_lock(false);
        Handle g_resource_limit_handles[ResourceLimitGroup_Count];
        spl::MemoryArrangement g_memory_arrangement = spl::MemoryArrangement_Standard;
        u64 g_system_memory_boost_size = 0;
        u64 g_extra_application_threads_available = 0;

        u64 g_resource_limits[ResourceLimitGroup_Count][svc::LimitableResource_Count] = {
            [ResourceLimitGroup_System] = {
                [svc::LimitableResource_PhysicalMemoryMax]      = 0,   /* Initialized by more complicated logic later. */
                [svc::LimitableResource_ThreadCountMax]         = 508,
                [svc::LimitableResource_EventCountMax]          = 600,
                [svc::LimitableResource_TransferMemoryCountMax] = 128,
                [svc::LimitableResource_SessionCountMax]        = 794,
            },
            [ResourceLimitGroup_Application] = {
                [svc::LimitableResource_PhysicalMemoryMax]      = 0,   /* Initialized by more complicated logic later. */
                [svc::LimitableResource_ThreadCountMax]         = 96,
                [svc::LimitableResource_EventCountMax]          = 0,
                [svc::LimitableResource_TransferMemoryCountMax] = 32,
                [svc::LimitableResource_SessionCountMax]        = 1,
            },
            [ResourceLimitGroup_Applet] = {
                [svc::LimitableResource_PhysicalMemoryMax]      = 0,   /* Initialized by more complicated logic later. */
                [svc::LimitableResource_ThreadCountMax]         = 96,
                [svc::LimitableResource_EventCountMax]          = 0,
                [svc::LimitableResource_TransferMemoryCountMax] = 32,
                [svc::LimitableResource_SessionCountMax]        = 5,
            },
        };

        u64 g_memory_resource_limits[spl::MemoryArrangement_Count][ResourceLimitGroup_Count] = {
            [spl::MemoryArrangement_Standard] = {
                [ResourceLimitGroup_System]      =  269_MB,
                [ResourceLimitGroup_Application] = 3285_MB,
                [ResourceLimitGroup_Applet]      =  535_MB,
            },
            [spl::MemoryArrangement_StandardForAppletDev] = {
                [ResourceLimitGroup_System]      =  481_MB,
                [ResourceLimitGroup_Application] = 2048_MB,
                [ResourceLimitGroup_Applet]      = 1560_MB,
            },
            [spl::MemoryArrangement_StandardForSystemDev] = {
                [ResourceLimitGroup_System]      =  328_MB,
                [ResourceLimitGroup_Application] = 3285_MB,
                [ResourceLimitGroup_Applet]      =  476_MB,
            },
            [spl::MemoryArrangement_Expanded] = {
                [ResourceLimitGroup_System]      =  653_MB,
                [ResourceLimitGroup_Application] = 4916_MB,
                [ResourceLimitGroup_Applet]      =  568_MB,
            },
            [spl::MemoryArrangement_ExpandedForAppletDev] = {
                [ResourceLimitGroup_System]      =  653_MB,
                [ResourceLimitGroup_Application] = 3285_MB,
                [ResourceLimitGroup_Applet]      = 2199_MB,
            },
        };

        /* Helpers. */
        Result SetMemoryResourceLimitLimitValue(ResourceLimitGroup group, u64 new_memory_limit) {
            const u64 old_memory_limit = g_resource_limits[group][svc::LimitableResource_PhysicalMemoryMax];
            g_resource_limits[group][svc::LimitableResource_PhysicalMemoryMax] = new_memory_limit;

            {
                /* If we fail, restore the old memory limit. */
                auto limit_guard = SCOPE_GUARD { g_resource_limits[group][svc::LimitableResource_PhysicalMemoryMax] = old_memory_limit; };
                R_TRY(svc::SetResourceLimitLimitValue(GetResourceLimitHandle(group), svc::LimitableResource_PhysicalMemoryMax, g_resource_limits[group][svc::LimitableResource_PhysicalMemoryMax]));
                limit_guard.Cancel();
            }

            return ResultSuccess();
        }

        Result SetResourceLimitLimitValues(ResourceLimitGroup group, u64 new_memory_limit) {
            /* First, set memory limit. */
            R_TRY(SetMemoryResourceLimitLimitValue(group, new_memory_limit));

            /* Set other limit values. */
            for (size_t i = 0; i < svc::LimitableResource_Count; i++) {
                const auto resource = LimitableResources[i];
                if (resource == svc::LimitableResource_PhysicalMemoryMax) {
                    continue;
                }
                R_TRY(svc::SetResourceLimitLimitValue(GetResourceLimitHandle(group), resource, g_resource_limits[group][resource]));
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
            for (size_t i = 0; i < svc::LimitableResource_Count; i++) {
                const auto resource = LimitableResources[i];

                s64 value = 0;
                while (true) {
                    R_ABORT_UNLESS(svc::GetResourceLimitCurrentValue(&value, reslimit_hnd, resource));
                    if (value == 0) {
                        break;
                    }
                    svc::SleepThread(1'000'000ul);
                }
            }
        }

        void WaitApplicationMemoryAvailable() {
            u64 value = 0;
            while (true) {
                R_ABORT_UNLESS(svc::GetSystemInfo(&value, svc::SystemInfoType_UsedPhysicalMemorySize, INVALID_HANDLE, svc::PhysicalMemorySystemInfo_Application));
                if (value == 0) {
                    break;
                }
                svc::SleepThread(1'000'000ul);
            }
        }

        bool IsKTraceEnabled() {
            if (!svc::IsKernelMesosphere()) {
                return false;
            }

            u64 value = 0;
            R_ABORT_UNLESS(svc::GetInfo(std::addressof(value), svc::InfoType_MesosphereMeta, INVALID_HANDLE, svc::MesosphereMetaInfo_IsKTraceEnabled));

            return value != 0;
        }

    }

    /* Resource API. */
    Result InitializeResourceManager() {
        /* Create resource limit handles. */
        for (size_t i = 0; i < ResourceLimitGroup_Count; i++) {
            if (i == ResourceLimitGroup_System) {
                u64 value = 0;
                R_ABORT_UNLESS(svc::GetInfo(&value, svc::InfoType_ResourceLimit, svc::InvalidHandle, 0));
                g_resource_limit_handles[i] = static_cast<svc::Handle>(value);
            } else {
                R_ABORT_UNLESS(svc::CreateResourceLimit(&g_resource_limit_handles[i]));
            }
        }

        /* Adjust resource limits based on hos firmware version. */
        const auto hos_version = hos::GetVersion();
        if (hos_version >= hos::Version_4_0_0) {
            /* 4.0.0 increased the system thread limit. */
            g_resource_limits[ResourceLimitGroup_System][svc::LimitableResource_ThreadCountMax] += ExtraSystemThreadCount400;
            /* 4.0.0 also took memory away from applet and gave it to system, for the Standard and StandardForSystemDev profiles. */
            g_memory_resource_limits[spl::MemoryArrangement_Standard][ResourceLimitGroup_System] += ExtraSystemMemorySize400;
            g_memory_resource_limits[spl::MemoryArrangement_Standard][ResourceLimitGroup_Applet] -= ExtraSystemMemorySize400;
            g_memory_resource_limits[spl::MemoryArrangement_StandardForSystemDev][ResourceLimitGroup_System] += ExtraSystemMemorySize400;
            g_memory_resource_limits[spl::MemoryArrangement_StandardForSystemDev][ResourceLimitGroup_Applet] -= ExtraSystemMemorySize400;
        }
        if (hos_version >= hos::Version_5_0_0) {
            /* 5.0.0 took more memory away from applet and gave it to system, for the Standard and StandardForSystemDev profiles. */
            g_memory_resource_limits[spl::MemoryArrangement_Standard][ResourceLimitGroup_System] += ExtraSystemMemorySize500;
            g_memory_resource_limits[spl::MemoryArrangement_Standard][ResourceLimitGroup_Applet] -= ExtraSystemMemorySize500;
            g_memory_resource_limits[spl::MemoryArrangement_StandardForSystemDev][ResourceLimitGroup_System] += ExtraSystemMemorySize500;
            g_memory_resource_limits[spl::MemoryArrangement_StandardForSystemDev][ResourceLimitGroup_Applet] -= ExtraSystemMemorySize500;
        }
        if (hos_version >= hos::Version_6_0_0) {
            /* 6.0.0 increased the system event and session limits. */
            g_resource_limits[ResourceLimitGroup_System][svc::LimitableResource_EventCountMax]   += ExtraSystemEventCount600;
            g_resource_limits[ResourceLimitGroup_System][svc::LimitableResource_SessionCountMax] += ExtraSystemSessionCount600;
        }
        if (hos_version >= hos::Version_9_2_0) {
            /* 9.2.0 increased the system session limit. */
            g_resource_limits[ResourceLimitGroup_System][svc::LimitableResource_SessionCountMax] += ExtraSystemSessionCount920;
        }

        /* 7.0.0+: Calculate the number of extra application threads available. */
        if (hos::GetVersion() >= hos::Version_7_0_0) {
            /* See how many threads we have available. */
            s64 total_threads_available = 0;
            R_ABORT_UNLESS(svc::GetResourceLimitLimitValue(&total_threads_available, GetResourceLimitHandle(ResourceLimitGroup_System), svc::LimitableResource_ThreadCountMax));

            /* See how many threads we're expecting. */
            const s64 total_threads_allocated = g_resource_limits[ResourceLimitGroup_System][svc::LimitableResource_ThreadCountMax] +
                                                       g_resource_limits[ResourceLimitGroup_Application][svc::LimitableResource_ThreadCountMax] +
                                                       g_resource_limits[ResourceLimitGroup_Applet][svc::LimitableResource_ThreadCountMax];

            /* Ensure we don't over-commit threads. */
            AMS_ABORT_UNLESS(total_threads_allocated <= total_threads_available);

            /* Set number of extra threads. */
            g_extra_application_threads_available = total_threads_available - total_threads_allocated;
        }

        /* Choose and initialize memory arrangement. */
        const bool use_dynamic_memory_arrangement = (hos_version >= hos::Version_6_0_0) || (svc::IsKernelMesosphere() && hos_version >= hos::Version_5_0_0);
        if (use_dynamic_memory_arrangement) {
            /* 6.0.0 retrieves memory limit information from the kernel, rather than using a hardcoded profile. */
            g_memory_arrangement = spl::MemoryArrangement_Dynamic;

            /* Get total memory available. */
            s64 total_memory = 0;
            R_ABORT_UNLESS(svc::GetResourceLimitLimitValue(&total_memory, GetResourceLimitHandle(ResourceLimitGroup_System), svc::LimitableResource_PhysicalMemoryMax));

            /* Get and save application + applet memory. */
            R_ABORT_UNLESS(svc::GetSystemInfo(&g_memory_resource_limits[spl::MemoryArrangement_Dynamic][ResourceLimitGroup_Application], svc::SystemInfoType_TotalPhysicalMemorySize, svc::InvalidHandle, svc::PhysicalMemorySystemInfo_Application));
            R_ABORT_UNLESS(svc::GetSystemInfo(&g_memory_resource_limits[spl::MemoryArrangement_Dynamic][ResourceLimitGroup_Applet],      svc::SystemInfoType_TotalPhysicalMemorySize, svc::InvalidHandle, svc::PhysicalMemorySystemInfo_Applet));

            const s64 application_size         = g_memory_resource_limits[spl::MemoryArrangement_Dynamic][ResourceLimitGroup_Application];
            const s64 applet_size              = g_memory_resource_limits[spl::MemoryArrangement_Dynamic][ResourceLimitGroup_Applet];
            const s64 reserved_non_system_size = (application_size + applet_size + ReservedMemorySize600);

            /* Ensure there's enough memory for the system region. */
            AMS_ABORT_UNLESS(reserved_non_system_size < total_memory);

            g_memory_resource_limits[spl::MemoryArrangement_Dynamic][ResourceLimitGroup_System] = total_memory - reserved_non_system_size;
        } else {
            /* Older system versions retrieve memory arrangement from spl, and use hardcoded profiles. */
            g_memory_arrangement = spl::GetMemoryArrangement();

            /* Adjust memory limits for atmosphere. */
            /* We take memory away from applet normally, but away from application on < 3.0.0 to avoid a rare hang on boot. */
            /* NOTE: On Version 5.0.0+, we cannot set the pools so simply. We must instead modify the kernel, which we do */
            /* via patches in fusee-secondary. */
            const size_t extra_memory_size = hos_version == hos::Version_5_0_0 ? ExtraSystemMemorySizeAtmosphere500 : ExtraSystemMemorySizeAtmosphere;
            const auto src_group = hos_version >= hos::Version_3_0_0 ? ResourceLimitGroup_Applet : ResourceLimitGroup_Application;
            for (size_t i = 0; i < spl::MemoryArrangement_Count; i++) {
                g_memory_resource_limits[i][ResourceLimitGroup_System] += extra_memory_size;
                g_memory_resource_limits[i][src_group] -= extra_memory_size;
            }

            /* If KTrace is enabled, account for that by subtracting the memory from the applet pool. */
            if (IsKTraceEnabled()) {
                constexpr size_t KTraceBufferSize = 16_MB;
                for (size_t i = 0; i < spl::MemoryArrangement_Count; i++) {
                    g_memory_resource_limits[i][ResourceLimitGroup_Applet] -= KTraceBufferSize;
                }
            }
        }

        /* Actually set resource limits. */
        {
            std::scoped_lock lk(g_resource_limit_lock);

            for (size_t group = 0; group < ResourceLimitGroup_Count; group++) {
                R_ABORT_UNLESS(SetResourceLimitLimitValues(static_cast<ResourceLimitGroup>(group), g_memory_resource_limits[g_memory_arrangement][group]));
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

            if (hos::GetVersion() >= hos::Version_5_0_0) {
                /* Starting in 5.0.0, PM does not allow for only one of the sets to fail. */
                if (boost_size < g_system_memory_boost_size) {
                    R_TRY(svc::SetUnsafeLimit(boost_size));
                    R_ABORT_UNLESS(SetMemoryResourceLimitLimitValue(ResourceLimitGroup_Application, new_app_size));
                } else {
                    R_TRY(SetMemoryResourceLimitLimitValue(ResourceLimitGroup_Application, new_app_size));
                    R_ABORT_UNLESS(svc::SetUnsafeLimit(boost_size));
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
        const s64 new_thread_count = g_resource_limits[ResourceLimitGroup_Application][svc::LimitableResource_ThreadCountMax] + g_extra_application_threads_available;
        R_TRY(svc::SetResourceLimitLimitValue(GetResourceLimitHandle(ResourceLimitGroup_Application), svc::LimitableResource_ThreadCountMax, new_thread_count));

        /* Record that we did so. */
        g_resource_limits[ResourceLimitGroup_Application][svc::LimitableResource_ThreadCountMax] = new_thread_count;
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
            if (hos::GetVersion() >= hos::Version_5_0_0) {
                WaitApplicationMemoryAvailable();
            }
        }
    }

    Result GetResourceLimitValues(s64 *out_cur, s64 *out_lim, ResourceLimitGroup group, svc::LimitableResource resource) {
        /* Do not allow out of bounds access. */
        AMS_ABORT_UNLESS(group < ResourceLimitGroup_Count);
        AMS_ABORT_UNLESS(resource < svc::LimitableResource_Count);

        const Handle reslimit_hnd = GetResourceLimitHandle(group);
        R_TRY(svc::GetResourceLimitCurrentValue(out_cur, reslimit_hnd, resource));
        R_TRY(svc::GetResourceLimitLimitValue(out_lim, reslimit_hnd, resource));

        return ResultSuccess();
    }

}
