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
#include "pm_spec.hpp"

namespace ams::pm::impl {

    namespace {

        constexpr svc::LimitableResource LimitableResources[] = {
            svc::LimitableResource_PhysicalMemoryMax,
            svc::LimitableResource_ThreadCountMax,
            svc::LimitableResource_EventCountMax,
            svc::LimitableResource_TransferMemoryCountMax,
            svc::LimitableResource_SessionCountMax,
        };

        /* Definitions for limit differences over time. */
        constexpr size_t ExtraSystemMemorySize400    = 10_MB;
        constexpr size_t ReservedMemorySize600       = 5_MB;

        /* Atmosphere always allocates extra memory for system usage. */
        constexpr size_t ExtraSystemMemorySizeAtmosphere    = 32_MB;

        /* Desired extra threads. */
        constexpr u64 BaseApplicationThreads  = 96;
        constexpr u64 BaseAppletThreads       = 96;
        constexpr u64 BaseSystemThreads       = 800 - BaseAppletThreads - BaseApplicationThreads;

        constexpr s64 ExtraSystemThreads      = 1024 - BaseSystemThreads;
        constexpr s64 ExtraApplicationThreads = 256 - BaseApplicationThreads;
        constexpr s64 ExtraAppletThreads      = 256 - BaseAppletThreads;

        static_assert(ExtraSystemThreads >= 0);
        static_assert(ExtraApplicationThreads >= 0);
        static_assert(ExtraAppletThreads >= 0);

        /* Globals. */
        constinit os::SdkMutex g_resource_limit_lock;
        constinit os::NativeHandle g_resource_limit_handles[ResourceLimitGroup_Count];
        constinit spl::MemoryArrangement g_memory_arrangement = spl::MemoryArrangement_Standard;
        constinit u64 g_extra_threads_available[ResourceLimitGroup_Count];

        constinit os::SdkMutex g_system_memory_boost_lock;
        constinit u64 g_system_memory_boost_size          = 0;
        constinit u64 g_system_memory_boost_size_for_mitm = 0;

        ALWAYS_INLINE u64 GetCurrentSystemMemoryBoostSize() {
            return g_system_memory_boost_size + g_system_memory_boost_size_for_mitm;
        }

        constinit u64 g_resource_limits[ResourceLimitGroup_Count][svc::LimitableResource_Count] = {
            [ResourceLimitGroup_System] = {
                [svc::LimitableResource_PhysicalMemoryMax]      = 0,   /* Initialized dynamically later. */
                [svc::LimitableResource_ThreadCountMax]         = BaseSystemThreads,
                [svc::LimitableResource_EventCountMax]          = 0,   /* Initialized dynamically later. */
                [svc::LimitableResource_TransferMemoryCountMax] = 0,   /* Initialized dynamically later. */
                [svc::LimitableResource_SessionCountMax]        = 0,   /* Initialized dynamically later. */
            },
            [ResourceLimitGroup_Application] = {
                [svc::LimitableResource_PhysicalMemoryMax]      = 0,   /* Initialized dynamically later. */
                [svc::LimitableResource_ThreadCountMax]         = BaseApplicationThreads,
                [svc::LimitableResource_EventCountMax]          = 0,
                [svc::LimitableResource_TransferMemoryCountMax] = 32,
                [svc::LimitableResource_SessionCountMax]        = 1,
            },
            [ResourceLimitGroup_Applet] = {
                [svc::LimitableResource_PhysicalMemoryMax]      = 0,   /* Initialized dynamically later. */
                [svc::LimitableResource_ThreadCountMax]         = BaseAppletThreads,
                [svc::LimitableResource_EventCountMax]          = 0,
                [svc::LimitableResource_TransferMemoryCountMax] = 32,
                [svc::LimitableResource_SessionCountMax]        = 5 + 1, /* Add a session for atmosphere's memlet system module. */
            },
        };

        constinit u64 g_memory_resource_limits[spl::MemoryArrangement_Count][ResourceLimitGroup_Count] = {
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

            /* Restore the old memory limit if we fail. */
            ON_RESULT_FAILURE { g_resource_limits[group][svc::LimitableResource_PhysicalMemoryMax] = old_memory_limit; };

            /* Set the resource limit. */
            R_RETURN(svc::SetResourceLimitLimitValue(GetResourceLimitHandle(group), svc::LimitableResource_PhysicalMemoryMax, g_resource_limits[group][svc::LimitableResource_PhysicalMemoryMax]));
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

            R_SUCCEED();
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
            const auto reslimit_hnd = GetResourceLimitHandle(group);
            for (size_t i = 0; i < svc::LimitableResource_Count; i++) {
                const auto resource = LimitableResources[i];

                s64 value = 0;
                while (true) {
                    R_ABORT_UNLESS(svc::GetResourceLimitCurrentValue(&value, reslimit_hnd, resource));
                    if (value == 0) {
                        break;
                    }
                    os::SleepThread(TimeSpan::FromMilliSeconds(1));
                }
            }
        }

        void WaitApplicationMemoryAvailable() {
            /* Get firmware version. */
            const auto fw_ver = hos::GetVersion();

            /* On 15.0.0+, pm considers application memory to be available if there is exactly 96 MB outstanding. */
            /* This is probably because this corresponds to the gameplay-recording memory. */
            constexpr u64 AllowedUsedApplicationMemory = 96_MB;

            /* Wait for memory to be available. */
            u64 value = 0;
            while (true) {
                R_ABORT_UNLESS(svc::GetSystemInfo(&value, svc::SystemInfoType_UsedPhysicalMemorySize, svc::InvalidHandle, svc::PhysicalMemorySystemInfo_Application));
                if (value == 0 || (fw_ver >= hos::Version_15_0_0 && value == AllowedUsedApplicationMemory)) {
                    break;
                }
                os::SleepThread(TimeSpan::FromMilliSeconds(1));
            }
        }

        bool IsKTraceEnabled() {
            u64 value = 0;
            R_ABORT_UNLESS(svc::GetInfo(std::addressof(value), svc::InfoType_MesosphereMeta, svc::InvalidHandle, svc::MesosphereMetaInfo_IsKTraceEnabled));

            return value != 0;
        }

        ALWAYS_INLINE Result BoostThreadResourceLimitLocked(ResourceLimitGroup group) {
            AMS_ASSERT(g_resource_limit_lock.IsLockedByCurrentThread());

            /* Set new limit. */
            const s64 new_thread_count = g_resource_limits[group][svc::LimitableResource_ThreadCountMax] + g_extra_threads_available[group];
            R_TRY(svc::SetResourceLimitLimitValue(GetResourceLimitHandle(group), svc::LimitableResource_ThreadCountMax, new_thread_count));

            /* Record that we did so. */
            g_resource_limits[group][svc::LimitableResource_ThreadCountMax] = new_thread_count;
            g_extra_threads_available[group] = 0;

            R_SUCCEED();
        }

        template<auto ImplFunction>
        ALWAYS_INLINE Result GetResourceLimitValueImpl(pm::ResourceLimitValue *out, ResourceLimitGroup group) {
            /* Sanity check group. */
            AMS_ABORT_UNLESS(group < ResourceLimitGroup_Count);

            /* Get handle. */
            const auto handle = GetResourceLimitHandle(group);

            /* Get values. */
            int64_t values[svc::LimitableResource_Count];
            R_ABORT_UNLESS(ImplFunction(std::addressof(values[svc::LimitableResource_PhysicalMemoryMax]),      handle, svc::LimitableResource_PhysicalMemoryMax));
            R_ABORT_UNLESS(ImplFunction(std::addressof(values[svc::LimitableResource_ThreadCountMax]),         handle, svc::LimitableResource_ThreadCountMax));
            R_ABORT_UNLESS(ImplFunction(std::addressof(values[svc::LimitableResource_EventCountMax]),          handle, svc::LimitableResource_EventCountMax));
            R_ABORT_UNLESS(ImplFunction(std::addressof(values[svc::LimitableResource_TransferMemoryCountMax]), handle, svc::LimitableResource_TransferMemoryCountMax));
            R_ABORT_UNLESS(ImplFunction(std::addressof(values[svc::LimitableResource_SessionCountMax]),        handle, svc::LimitableResource_SessionCountMax));

            /* Set to output. */
            out->physical_memory       = values[svc::LimitableResource_PhysicalMemoryMax];
            out->thread_count          = values[svc::LimitableResource_ThreadCountMax];
            out->event_count           = values[svc::LimitableResource_EventCountMax];
            out->transfer_memory_count = values[svc::LimitableResource_TransferMemoryCountMax];
            out->session_count         = values[svc::LimitableResource_SessionCountMax];

            R_SUCCEED();
        }

        Result BoostSystemMemoryResourceLimitLocked(u64 normal_boost, u64 mitm_boost) {
            /* Check pre-conditions. */
            AMS_ASSERT(g_system_memory_boost_lock.IsLockedByCurrentThread());

            /* Determine total boost. */
            const u64 boost_size = normal_boost + mitm_boost;

            /* Don't allow all application memory to be taken away. */
            R_UNLESS(boost_size < g_memory_resource_limits[g_memory_arrangement][ResourceLimitGroup_Application], pm::ResultInvalidSize());

            const u64 new_app_size = g_memory_resource_limits[g_memory_arrangement][ResourceLimitGroup_Application] - boost_size;
            {
                std::scoped_lock lk(g_resource_limit_lock);

                const auto cur_boost_size = GetCurrentSystemMemoryBoostSize();

                if (hos::GetVersion() >= hos::Version_5_0_0) {
                    /* Starting in 5.0.0, PM does not allow for only one of the sets to fail. */
                    if (boost_size < cur_boost_size) {
                        R_TRY(svc::SetUnsafeLimit(boost_size));
                        R_ABORT_UNLESS(SetMemoryResourceLimitLimitValue(ResourceLimitGroup_Application, new_app_size));
                    }  else if (boost_size > cur_boost_size) {
                        R_TRY(SetMemoryResourceLimitLimitValue(ResourceLimitGroup_Application, new_app_size));
                        R_ABORT_UNLESS(svc::SetUnsafeLimit(boost_size));
                    } else {
                        /* If the boost size is equal, there's nothing to do. */
                    }
                } else {
                    const u64 new_sys_size = g_memory_resource_limits[g_memory_arrangement][ResourceLimitGroup_System] + boost_size;
                    if (boost_size < cur_boost_size) {
                        R_TRY(SetMemoryResourceLimitLimitValue(ResourceLimitGroup_System,      new_sys_size));
                        R_TRY(SetMemoryResourceLimitLimitValue(ResourceLimitGroup_Application, new_app_size));
                    } else if (boost_size > cur_boost_size) {
                        R_TRY(SetMemoryResourceLimitLimitValue(ResourceLimitGroup_Application, new_app_size));
                        R_TRY(SetMemoryResourceLimitLimitValue(ResourceLimitGroup_System,      new_sys_size));
                    } else {
                        /* If the boost size is equal, there's nothing to do. */
                    }
                }

                g_system_memory_boost_size         = normal_boost;
                g_system_memory_boost_size_for_mitm = mitm_boost;
            }

            R_SUCCEED();
        }

    }

    Result InitializeSpec() {
        /* Create resource limit handles. */
        for (size_t i = 0; i < ResourceLimitGroup_Count; i++) {
            if (i == ResourceLimitGroup_System) {
                u64 value = 0;
                R_ABORT_UNLESS(svc::GetInfo(&value, svc::InfoType_ResourceLimit, svc::InvalidHandle, 0));
                g_resource_limit_handles[i] = static_cast<svc::Handle>(value);
            } else {
                R_ABORT_UNLESS(svc::CreateResourceLimit(g_resource_limit_handles + i));
            }
        }

        /* Adjust memory limits based on hos firmware version. */
        const auto hos_version = hos::GetVersion();
        if (hos_version >= hos::Version_4_0_0) {
            /* 4.0.0 took memory away from applet and gave it to system, for the Standard and StandardForSystemDev profiles. */
            g_memory_resource_limits[spl::MemoryArrangement_Standard][ResourceLimitGroup_System] += ExtraSystemMemorySize400;
            g_memory_resource_limits[spl::MemoryArrangement_Standard][ResourceLimitGroup_Applet] -= ExtraSystemMemorySize400;
            g_memory_resource_limits[spl::MemoryArrangement_StandardForSystemDev][ResourceLimitGroup_System] += ExtraSystemMemorySize400;
            g_memory_resource_limits[spl::MemoryArrangement_StandardForSystemDev][ResourceLimitGroup_Applet] -= ExtraSystemMemorySize400;
        }

        /* Determine system resource counts. */
        {
            /* Get the total resource counts. */
            s64 total_events, total_transfer_memories, total_sessions;
            R_ABORT_UNLESS(svc::GetResourceLimitLimitValue(std::addressof(total_events), GetResourceLimitHandle(ResourceLimitGroup_System), svc::LimitableResource_EventCountMax));
            R_ABORT_UNLESS(svc::GetResourceLimitLimitValue(std::addressof(total_transfer_memories), GetResourceLimitHandle(ResourceLimitGroup_System), svc::LimitableResource_TransferMemoryCountMax));
            R_ABORT_UNLESS(svc::GetResourceLimitLimitValue(std::addressof(total_sessions), GetResourceLimitHandle(ResourceLimitGroup_System), svc::LimitableResource_SessionCountMax));

            /* Determine system counts. */
            const s64 sys_events            = total_events - (g_resource_limits[ResourceLimitGroup_Application][svc::LimitableResource_EventCountMax] + g_resource_limits[ResourceLimitGroup_Applet][svc::LimitableResource_EventCountMax]);
            const s64 sys_transfer_memories = total_transfer_memories - (g_resource_limits[ResourceLimitGroup_Application][svc::LimitableResource_TransferMemoryCountMax] + g_resource_limits[ResourceLimitGroup_Applet][svc::LimitableResource_TransferMemoryCountMax]);
            const s64 sys_sessions          = total_sessions - (g_resource_limits[ResourceLimitGroup_Application][svc::LimitableResource_SessionCountMax] + g_resource_limits[ResourceLimitGroup_Applet][svc::LimitableResource_SessionCountMax]);

            /* Check system counts. */
            AMS_ABORT_UNLESS(sys_events >= 0);
            AMS_ABORT_UNLESS(sys_transfer_memories >= 0);
            AMS_ABORT_UNLESS(sys_sessions >= 0);

            /* Set system counts. */
            g_resource_limits[ResourceLimitGroup_System][svc::LimitableResource_EventCountMax]          = sys_events;
            g_resource_limits[ResourceLimitGroup_System][svc::LimitableResource_TransferMemoryCountMax] = sys_transfer_memories;
            g_resource_limits[ResourceLimitGroup_System][svc::LimitableResource_SessionCountMax]        = sys_sessions;
        }

        /* Determine extra application threads. */
        {
            /* Get total threads available. */
            s64 total_threads;
            R_ABORT_UNLESS(svc::GetResourceLimitLimitValue(std::addressof(total_threads), GetResourceLimitHandle(ResourceLimitGroup_System), svc::LimitableResource_ThreadCountMax));

            /* Check that we have enough threads. */
            const s64 required_threads = g_resource_limits[ResourceLimitGroup_System][svc::LimitableResource_ThreadCountMax] + g_resource_limits[ResourceLimitGroup_Application][svc::LimitableResource_ThreadCountMax] + g_resource_limits[ResourceLimitGroup_Applet][svc::LimitableResource_ThreadCountMax];
            AMS_ABORT_UNLESS(total_threads >= required_threads);

            /* Set the number of extra threads. */
            const s64 extra_threads = total_threads - required_threads;
            if constexpr (true /* TODO: Should we expose the old "all extra threads are application" behavior? Seems pointless. */) {
                if (extra_threads > 0) {
                    /* If we have any extra threads at all, require that we have enough. */
                    AMS_ABORT_UNLESS(extra_threads >= (ExtraSystemThreads + ExtraApplicationThreads + ExtraAppletThreads));

                    g_extra_threads_available[ResourceLimitGroup_System]      += ExtraSystemThreads;
                    g_extra_threads_available[ResourceLimitGroup_Application] += ExtraApplicationThreads;
                    g_extra_threads_available[ResourceLimitGroup_Applet]      += ExtraAppletThreads;
                }
            } else {
                g_extra_threads_available[ResourceLimitGroup_Application] = extra_threads;
            }
        }

        /* Choose and initialize memory arrangement. */
        const bool use_dynamic_memory_arrangement = (hos_version >= hos::Version_5_0_0);
        if (use_dynamic_memory_arrangement) {
            /* 6.0.0 retrieves memory limit information from the kernel, rather than using a hardcoded profile. */
            g_memory_arrangement = spl::MemoryArrangement_Dynamic;

            /* Get total memory available. */
            s64 total_memory = 0;
            R_ABORT_UNLESS(svc::GetResourceLimitLimitValue(&total_memory, GetResourceLimitHandle(ResourceLimitGroup_System), svc::LimitableResource_PhysicalMemoryMax));

            /* Get and save application + applet memory. */
            R_ABORT_UNLESS(svc::GetSystemInfo(std::addressof(g_memory_resource_limits[spl::MemoryArrangement_Dynamic][ResourceLimitGroup_Application]), svc::SystemInfoType_TotalPhysicalMemorySize, svc::InvalidHandle, svc::PhysicalMemorySystemInfo_Application));
            R_ABORT_UNLESS(svc::GetSystemInfo(std::addressof(g_memory_resource_limits[spl::MemoryArrangement_Dynamic][ResourceLimitGroup_Applet]),      svc::SystemInfoType_TotalPhysicalMemorySize, svc::InvalidHandle, svc::PhysicalMemorySystemInfo_Applet));

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
            const size_t extra_memory_size = ExtraSystemMemorySizeAtmosphere;
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

        R_SUCCEED();
    }

    Result BoostSystemMemoryResourceLimit(u64 boost_size) {
        /* Ensure only one boost change happens at a time. */
        std::scoped_lock lk(g_system_memory_boost_lock);

        /* Boost to the appropriate total amount. */
        R_RETURN(BoostSystemMemoryResourceLimitLocked(boost_size, g_system_memory_boost_size_for_mitm));
    }

    Result BoostSystemMemoryResourceLimitForMitm(u64 boost_size) {
        /* Ensure only one boost change happens at a time. */
        std::scoped_lock lk(g_system_memory_boost_lock);

        /* Boost to the appropriate total amount. */
        R_RETURN(BoostSystemMemoryResourceLimitLocked(g_system_memory_boost_size, boost_size));
    }

    Result BoostApplicationThreadResourceLimit() {
        std::scoped_lock lk(g_resource_limit_lock);

        /* Boost the limit. */
        R_TRY(BoostThreadResourceLimitLocked(ResourceLimitGroup_Application));

        R_SUCCEED();
    }

    Result BoostSystemThreadResourceLimit() {
        std::scoped_lock lk(g_resource_limit_lock);

        /* Boost the limits. */
        R_TRY(BoostThreadResourceLimitLocked(ResourceLimitGroup_Applet));
        R_TRY(BoostThreadResourceLimitLocked(ResourceLimitGroup_System));

        R_SUCCEED();
    }

    os::NativeHandle GetResourceLimitHandle(ResourceLimitGroup group) {
        return g_resource_limit_handles[group];
    }

    os::NativeHandle GetResourceLimitHandle(const ldr::ProgramInfo *info) {
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

    Result GetResourceLimitCurrentValue(pm::ResourceLimitValue *out, ResourceLimitGroup group) {
        R_RETURN(GetResourceLimitValueImpl<::ams::svc::GetResourceLimitCurrentValue>(out, group));
    }

    Result GetResourceLimitPeakValue(pm::ResourceLimitValue *out, ResourceLimitGroup group) {
        R_RETURN(GetResourceLimitValueImpl<::ams::svc::GetResourceLimitPeakValue>(out, group));
    }

    Result GetResourceLimitLimitValue(pm::ResourceLimitValue *out, ResourceLimitGroup group) {
        R_RETURN(GetResourceLimitValueImpl<::ams::svc::GetResourceLimitLimitValue>(out, group));
    }

    Result GetResourceLimitValues(s64 *out_cur, s64 *out_lim, ResourceLimitGroup group, svc::LimitableResource resource) {
        /* Do not allow out of bounds access. */
        AMS_ABORT_UNLESS(group < ResourceLimitGroup_Count);
        AMS_ABORT_UNLESS(resource < svc::LimitableResource_Count);

        const auto reslimit_hnd = GetResourceLimitHandle(group);
        R_TRY(svc::GetResourceLimitCurrentValue(out_cur, reslimit_hnd, resource));
        R_TRY(svc::GetResourceLimitLimitValue(out_lim, reslimit_hnd, resource));

        R_SUCCEED();
    }

}
