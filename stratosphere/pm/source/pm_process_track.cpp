#include <switch.h>
#include <stratosphere.hpp>
#include "pm_process_track.hpp"

static SystemEvent *g_process_event = NULL;
static SystemEvent *g_debug_title_event = NULL;
static SystemEvent *g_debug_application_event = NULL;

static const u64 g_memory_resource_limits[5][3] = {
    {0x010D00000ULL, 0x0CD500000ULL, 0x021700000ULL},
    {0x01E100000ULL, 0x080000000ULL, 0x061800000ULL},
    {0x014800000ULL, 0x0CD500000ULL, 0x01DC00000ULL},
    {0x028D00000ULL, 0x133400000ULL, 0x023800000ULL},
    {0x028D00000ULL, 0x0CD500000ULL, 0x089700000ULL}
};

/* These are the limits for LimitableResources. */
/* Memory, Threads, Events, TransferMemories, Sessions. */
static u64 g_resource_limits[3][5] = {
    {0x0, 0x1FC, 0x258, 0x80, 0x31A},
    {0x0, 0x60, 0x0, 0x20, 0x1},
    {0x0, 0x60, 0x0, 0x20, 0x5},
};

static Handle g_resource_limit_handles[5] = {0};

void ProcessTracking::Initialize() {
    /* TODO: Setup ResourceLimit values, create MainLoop thread. */
    g_process_event = new SystemEvent(&IEvent::PanicCallback);
    g_debug_title_event = new SystemEvent(&IEvent::PanicCallback);
    g_debug_application_event = new SystemEvent(&IEvent::PanicCallback);
    
    /* Get memory limits. */
    u64 memory_arrangement;
    if (R_FAILED(splGetConfig(SplConfigItem_MemoryArrange, &memory_arrangement))) {
        /* TODO: panic. */
    }
    memory_arrangement &= 0x3F;
    int memory_limit_type;
    switch (memory_arrangement) {
        case 2:
            memory_limit_type = 1;
            break;
        case 3:
            memory_limit_type = 2;
            break;
        case 17:
            memory_limit_type = 3;
            break;
        case 18:
            memory_limit_type = 4;
            break;
        default:
            memory_limit_type = 0;
            break;
    }
    for (unsigned int i = 0; i < 3; i++) {
        g_resource_limits[i][0] = g_memory_resource_limits[memory_limit_type][i];
    }
    
    /* Create resource limits. */
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
        for (unsigned int r = 0; r < 5; r++) {
            if (R_FAILED(svcSetResourceLimitLimitValue(g_resource_limit_handles[i], r, g_resource_limits[i][r]))) {
                /* TODO: Panic. */
            }
        }
    }
}

void ProcessTracking::MainLoop(void *arg) {
    /* TODO */
    while (true) {
        /* PM, as a sysmodule, is basically just a while loop. */
        
        /* TODO: Properly implement this. */
        svcSleepThread(100000ULL);
        
        /* This is that while loop. */
    }
}