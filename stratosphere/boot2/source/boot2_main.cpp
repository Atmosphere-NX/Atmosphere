#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <malloc.h>

#include <switch.h>
#include <stratosphere.hpp>

#define PMC_BASE 0x7000E400

extern "C" {
    extern u32 __start__;

    u32 __nx_applet_type = AppletType_None;

    #define INNER_HEAP_SIZE 0x200000
    size_t nx_inner_heap_size = INNER_HEAP_SIZE;
    char   nx_inner_heap[INNER_HEAP_SIZE];
    
    void __libnx_initheap(void);
    void __appInit(void);
    void __appExit(void);
}

void __libnx_initheap(void) {
	void*  addr = nx_inner_heap;
	size_t size = nx_inner_heap_size;

	/* Newlib */
	extern char* fake_heap_start;
	extern char* fake_heap_end;

	fake_heap_start = (char*)addr;
	fake_heap_end   = (char*)addr + size;
}

void __appInit(void) {
    Result rc;

    rc = smInitialize();
    if (R_FAILED(rc))
        fatalSimple(MAKERESULT(Module_Libnx, LibnxError_InitFail_SM));
    
    rc = fsInitialize();
    if (R_FAILED(rc))
        fatalSimple(MAKERESULT(Module_Libnx, LibnxError_InitFail_FS));
    
    rc = pmshellInitialize();
    if (R_FAILED(rc))
        fatalSimple(0xCAFE << 4 | 1);
}

void __appExit(void) {
    /* Cleanup services. */
    pmshellExit(); 
    fsExit();
    smExit();
}

void LaunchTitle(u64 title_id, u64 storage_id, u32 launch_flags, u64 *pid) {
    u64 local_pid;
    Result rc = pmshellLaunchProcess(launch_flags, title_id, storage_id, &local_pid);
    switch (rc) {
        case 0xCE01:
            /* Out of resource! */
            /* TODO: Panic(). */
            break;
        case 0xDE01:
            /* Out of memory! */
            /* TODO: Panic(). */
            break;
        case 0xD001:
            /* Limit Reached! */
            /* TODO: Panic(). */
            break;
        default:
            /* We don't care about other issues. */
            break;
    }
    if (pid) {
        *pid = local_pid;
    }
}

bool ShouldForceMaintenanceMode() {
    /* TODO: Contact set:sys, retrieve boot!force_maintenance, read plus/minus buttons. */
    return false;
}

static const std::tuple<u64, bool> g_additional_launch_programs[] = {
    {0x0100000000000023, true},  /* am */
    {0x0100000000000019, true},  /* nvservices */
    {0x010000000000001C, true},  /* nvnflinger */
    {0x010000000000002D, true},  /* vi */
    {0x010000000000001F, true},  /* ns */
    {0x0100000000000015, true},  /* lm */
    {0x010000000000001B, true},  /* ppc */
    {0x0100000000000010, true},  /* ptm */
    {0x0100000000000013, true},  /* hid */
    {0x0100000000000014, true},  /* audio */
    {0x0100000000000029, true},  /* lbl */
    {0x0100000000000016, true},  /* wlan */
    {0x010000000000000B, true},  /* bluetooth */
    {0x0100000000000012, true},  /* bsdsockets */
    {0x010000000000000F, true},  /* nifm */
    {0x0100000000000018, true},  /* ldn */
    {0x010000000000001E, true},  /* account */
    {0x010000000000000E, false}, /* friends */
    {0x0100000000000020, true},  /* nfc */
    {0x010000000000003C, true},  /* jpegdec */
    {0x0100000000000022, true},  /* capsrv */
    {0x0100000000000024, true},  /* ssl */
    {0x0100000000000025, true},  /* nim */
    {0x010000000000000C, false}, /* bcat */
    {0x010000000000002B, true},  /* erpt */
    {0x0100000000000033, true},  /* es */
    {0x010000000000002E, true},  /* pctl */
    {0x010000000000002A, true},  /* btm */
    {0x0100000000000030, false}, /* eupld */
    {0x0100000000000031, true},  /* glue */
    {0x0100000000000032, true},  /* eclct */
    {0x010000000000002F, false}, /* npns */
    {0x0100000000000034, true},  /* fatal */
    {0x0100000000000037, true},  /* ro */
    {0x0100000000000038, true},  /* doesn't exist on retail systems */
    {0x0100000000000039, true},  /* sdb */
    {0x010000000000003A, true},  /* migration */
    {0x0100000000000035, true},  /* grc */
};

int main(int argc, char **argv)
{
    consoleDebugInit(debugDevice_SVC);
     
    /* NOTE: Order for initial programs is modified. */
    /* This is to launch things required for Loader to mount SD card ASAP. */
    
    /* Launch psc. */
    LaunchTitle(0x0100000000000021, 3, 0, NULL);
    /* Launch bus. */
    LaunchTitle(0x010000000000000A, 3, 0, NULL);
    /* Launch pcv. */
    LaunchTitle(0x010000000000001A, 3, 0, NULL);
    /* Launch settings. */
    LaunchTitle(0x0100000000000009, 3, 0, NULL);
    /* Launch usb. */
    LaunchTitle(0x0100000000000006, 3, 0, NULL);
    /* Launch pcie. */
    LaunchTitle(0x010000000000001D, 3, 0, NULL);
    /* Launch tma. */
    LaunchTitle(0x0100000000000007, 3, 0, NULL);
    
    bool maintenance = ShouldForceMaintenanceMode();
    for (auto &launch_program : g_additional_launch_programs) {
        if (!maintenance || std::get<bool>(launch_program)) {
            LaunchTitle(std::get<u64>(launch_program), 3, 0, NULL);
        }
    }
       
    return 0;
}
