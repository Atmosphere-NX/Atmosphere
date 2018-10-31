/*
 * Copyright (c) 2018 Atmosphère-NX
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
 
#include <switch.h>
#include <stratosphere.hpp>

#include "pm_resource_limits.hpp"

/* Give 24 MiB to SYSTEM from Applet. This allows for more custom sysmodules. */
#define ATMOSPHERE_EXTRA_SYSTEM_MEMORY_FOR_SYSMODULES 0x1800000ULL

/* Memory that system, application, applet are allowed to allocate. */
static const u64 g_memory_resource_limits_deprecated[5][3] = {
    {0x010D00000ULL, 0x0CD500000ULL, 0x021700000ULL},
    {0x01E100000ULL, 0x080000000ULL, 0x061800000ULL},
    {0x014800000ULL, 0x0CD500000ULL, 0x01DC00000ULL},
    {0x028D00000ULL, 0x133400000ULL, 0x023800000ULL},
    {0x028D00000ULL, 0x0CD500000ULL, 0x089700000ULL}
};

/* These limits were altered in 4.x, which introduced ResourceLimitUtils::BoostSystemMemoryResourceLimit(). */
static const u64 g_memory_resource_limits_4x[5][3] = {
    {0x011700000ULL, 0x0CD500000ULL, 0x020D00000ULL}, /* 10 MB was taken from applet and given to system. */
    {0x01E100000ULL, 0x080000000ULL, 0x061800000ULL}, /* No changes. */
    {0x015200000ULL, 0x0CD500000ULL, 0x01D200000ULL}, /* 10 MB was taken from applet and given to system. */
    {0x028D00000ULL, 0x133400000ULL, 0x023800000ULL}, /* No changes. */
    {0x028D00000ULL, 0x0CD500000ULL, 0x089700000ULL}  /* No changes. */
};

/* These are the limits for LimitableResources. */
/* Memory, Threads, Events, TransferMemories, Sessions. */
static u64 g_resource_limits_deprecated[3][5] = {
    {0x0, 0x1FC, 0x258, 0x80, 0x31A},
    {0x0, 0x60, 0x0, 0x20, 0x1},
    {0x0, 0x60, 0x0, 0x20, 0x5},
};

/* 4.x boosted the number of threads that system modules are allowed to make. */
static u64 g_resource_limits_4x[3][5] = {
    {0x0, 0x260, 0x258, 0x80, 0x31A},
    {0x0, 0x60, 0x0, 0x20, 0x1},
    {0x0, 0x60, 0x0, 0x20, 0x5},
};

/* 6.x boosted the number of events and sessions that system modules are allowed to make. */
static u64 g_resource_limits_6x[3][5] = {
    {0x0, 0x260, 0x2BC, 0x80, 0x37E},
    {0x0, 0x60, 0x0, 0x20, 0x1},
    {0x0, 0x60, 0x0, 0x20, 0x5},
};

static Handle g_resource_limit_handles[3] = {0};


static u64 g_memory_resource_limits[6][3] = {0};
static u64 g_resource_limits[3][5] = {0};
static int g_memory_limit_type;
static u64 g_system_boost_size = 0;

/* Tries to set Resource limits for a category. */
static Result SetResourceLimits(ResourceLimitUtils::ResourceLimitCategory category, u64 new_memory_size) {
    Result rc = 0;
    u64 old_memory_size = g_resource_limits[category][LimitableResource_Memory];
    g_resource_limits[category][LimitableResource_Memory] = new_memory_size;
    for (unsigned int r = 0; r < 5; r++) {
        if (R_FAILED((rc = svcSetResourceLimitLimitValue(g_resource_limit_handles[category], (LimitableResource)r, g_resource_limits[category][r])))) {
            g_resource_limits[category][LimitableResource_Memory] = old_memory_size;
            return rc;
        }
    }
    return rc;
}


static Result SetNewMemoryResourceLimit(ResourceLimitUtils::ResourceLimitCategory category, u64 new_memory_size) {
    Result rc = 0;
    u64 old_memory_size = g_resource_limits[category][LimitableResource_Memory];
    g_resource_limits[category][LimitableResource_Memory] = new_memory_size;
    if (R_FAILED((rc = svcSetResourceLimitLimitValue(g_resource_limit_handles[category], LimitableResource_Memory, g_resource_limits[category][LimitableResource_Memory])))) {
        g_resource_limits[category][LimitableResource_Memory] = old_memory_size;
    }
    return rc;
}

void ResourceLimitUtils::InitializeLimits() {
    /* Create Resource Limits. */
    for (unsigned int i = 0; i < 3; i++) {
        if (i > 0) {
            if (R_FAILED(svcCreateResourceLimit(&g_resource_limit_handles[i]))) {
                /* TODO: Panic. */
            }
        } else {
            u64 out = 0;
            if (R_FAILED(svcGetInfo(&out, 9, 0, 0))) {
                /* TODO: Panic. */
            }
            g_resource_limit_handles[i] = (Handle)out;
        }
    }
    /* Set global aliases. */
    if (kernelAbove600()) {
        /* 6.0.0 did away with hardcoded memory resource limit values. */
        memcpy(&g_resource_limits, &g_resource_limits_6x, sizeof(g_resource_limits));
    } else if (kernelAbove400()) {
        memcpy(&g_memory_resource_limits, &g_memory_resource_limits_4x, sizeof(g_memory_resource_limits_4x));
        memcpy(&g_resource_limits, &g_resource_limits_4x, sizeof(g_resource_limits));
    } else {
        memcpy(&g_memory_resource_limits, &g_memory_resource_limits_deprecated, sizeof(g_memory_resource_limits_deprecated));
        memcpy(&g_resource_limits, &g_resource_limits_deprecated, sizeof(g_resource_limits));
    }
    
    if (kernelAbove600()) {
        /* NOTE: 5 is a fake type, official code does not do this. */
        /* This is done for ease of backwards compatibility. */
        g_memory_limit_type = 5;
        
        /* Starting 6.x, 5 MB of memory is always reserved for system. */
        const u64 reserved_system_size_6x = 0x500000;
                
        /* Get total memory available. */
        u64 total_memory = 0;
        if (R_FAILED(svcGetResourceLimitLimitValue(&total_memory, g_resource_limit_handles[0], LimitableResource_Memory))) {
            /* TODO: Panic. */
        }
        
        /* Get and save application + applet memory. */
        if (R_FAILED(svcGetSystemInfo(&g_memory_resource_limits[g_memory_limit_type][1], 0, 0, 0)) ||
            R_FAILED(svcGetSystemInfo(&g_memory_resource_limits[g_memory_limit_type][2], 0, 0, 1))) {
                /* TODO: Panic. */
            }
        
        const u64 application_size = g_memory_resource_limits[g_memory_limit_type][1];
        const u64 applet_size = g_memory_resource_limits[g_memory_limit_type][2];
        const u64 reserved_nonsys_size = (application_size + applet_size + reserved_system_size_6x);
            
        /* Ensure there's enough memory for system region. */
        if (reserved_nonsys_size > total_memory) {
            /* TODO: Panic. */
        }
        
        /* Set System memory. */
        g_memory_resource_limits[g_memory_limit_type][0] = total_memory - (reserved_nonsys_size);
    } else {
        /* Get memory limits. */
        u64 memory_arrangement;
        if (R_FAILED(splGetConfig(SplConfigItem_MemoryArrange, &memory_arrangement))) {
            /* TODO: panic. */
        }
        memory_arrangement &= 0x3F;
        switch (memory_arrangement) {
            case 2:
                g_memory_limit_type = 1;
                break;
            case 3:
                g_memory_limit_type = 2;
                break;
            case 17:
                g_memory_limit_type = 3;
                break;
            case 18:
                g_memory_limit_type = 4;
                break;
            default:
                g_memory_limit_type = 0;
                break;
        }
    }

    /* Atmosphere: Allocate extra memory (24 MiB) to SYSTEM away from Applet. */
    for (unsigned int i = 0; i < 6; i++) {
        g_memory_resource_limits[i][0] += ATMOSPHERE_EXTRA_SYSTEM_MEMORY_FOR_SYSMODULES;
        /* On < 4.0.0, taking from application instead of applet fixes a rare hang on boot. */
        if (kernelAbove400()) {
            g_memory_resource_limits[i][2] -= ATMOSPHERE_EXTRA_SYSTEM_MEMORY_FOR_SYSMODULES;
        } else {
            g_memory_resource_limits[i][1] -= ATMOSPHERE_EXTRA_SYSTEM_MEMORY_FOR_SYSMODULES;
        }
    }

    /* Set resource limits. */
    for (unsigned int i = 0; i < 3; i++) {
        g_resource_limits[i][LimitableResource_Memory] = g_memory_resource_limits[g_memory_limit_type][i];
        if (R_FAILED(SetResourceLimits((ResourceLimitCategory)i, g_memory_resource_limits[g_memory_limit_type][i]))) {
            /* TODO: Panic. */
        }
    }
}

void ResourceLimitUtils::EnsureApplicationResourcesAvailable() {
    Handle application_reslimit_h = g_resource_limit_handles[1];
    for (unsigned int i = 0; i < 5; i++) {
        u64 result;
        do {
            if (R_FAILED(svcGetResourceLimitCurrentValue(&result, application_reslimit_h, (LimitableResource)i))) {
                return;
            }
            svcSleepThread(1000000ULL);
        } while (result);
    }
    if (kernelAbove500()) {
        u64 result;
        do {
            if (R_FAILED(svcGetSystemInfo(&result, 1, 0, 0))) {
                /* TODO: Panic. */
            }
            svcSleepThread(1000000ULL);
        } while (result);
    }
}

Handle ResourceLimitUtils::GetResourceLimitHandle(u16 application_type) {
    if ((application_type & 3) == 1) {
        return g_resource_limit_handles[1];
    } else {
        return g_resource_limit_handles[2 * ((application_type & 3) == 2)];
    }
}

Handle ResourceLimitUtils::GetResourceLimitHandleByCategory(ResourceLimitCategory category) {
    return g_resource_limit_handles[category];
}

Result ResourceLimitUtils::BoostSystemMemoryResourceLimit(u64 boost_size) {
    Result rc = 0;
    if (boost_size > g_memory_resource_limits[g_memory_limit_type][ResourceLimitCategory_Application]) {
        return 0xC0F;
    }
    u64 app_size = g_memory_resource_limits[g_memory_limit_type][ResourceLimitCategory_Application] - boost_size;
    if (kernelAbove500()) {
        if (boost_size < g_system_boost_size) {
            if (R_FAILED((rc = svcSetUnsafeLimit(boost_size)))) {
                return rc;
            }
            if (R_FAILED((rc = SetNewMemoryResourceLimit(ResourceLimitCategory_Application, app_size)))) {
                return rc;
            }
        } else {
            if (R_FAILED((rc = SetNewMemoryResourceLimit(ResourceLimitCategory_Application, app_size)))) {
                return rc;
            }
            if (R_FAILED((rc = svcSetUnsafeLimit(boost_size)))) {
                return rc;
            }
        }
    } else if (kernelAbove400()) {
        u64 sys_size = g_memory_resource_limits[g_memory_limit_type][ResourceLimitCategory_System] + boost_size;
        if (boost_size < g_system_boost_size) {
            if (R_FAILED((rc = SetResourceLimits(ResourceLimitCategory_System, sys_size)))) {
                return rc;
            }
            if (R_FAILED((rc = SetResourceLimits(ResourceLimitCategory_Application, app_size)))) {
                return rc;
            }
        } else {
            if (R_FAILED((rc = SetResourceLimits(ResourceLimitCategory_Application, app_size)))) {
                return rc;
            }
            if (R_FAILED((rc = SetResourceLimits(ResourceLimitCategory_System, sys_size)))) {
                return rc;
            }
        }
    } else {
        rc = 0xF601;
    }
    if (R_SUCCEEDED(rc)) {
        g_system_boost_size = boost_size;
    }
    return rc;
}
