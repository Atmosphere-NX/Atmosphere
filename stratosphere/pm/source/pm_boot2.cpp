#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <malloc.h>

#include <switch.h>
#include <stratosphere.hpp>
#include "pm_boot2.hpp"
#include "pm_registration.hpp"

static void LaunchTitle(Boot2KnownTitleId title_id, FsStorageId storage_id, u32 launch_flags, u64 *pid) {
    u64 local_pid;
    
    Result rc = Registration::LaunchProcessByTidSid(Registration::TidSid{(u64)title_id, storage_id}, launch_flags, &local_pid);
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

static bool ShouldForceMaintenanceMode() {
    /* TODO: Contact set:sys, retrieve boot!force_maintenance, read plus/minus buttons. */
    return false;
}

static const std::tuple<Boot2KnownTitleId, bool> g_additional_launch_programs[] = {
    {Boot2KnownTitleId::am, true},          /* am */
    {Boot2KnownTitleId::nvservices, true},  /* nvservices */
    {Boot2KnownTitleId::nvnflinger, true},  /* nvnflinger */
    {Boot2KnownTitleId::vi, true},          /* vi */
    {Boot2KnownTitleId::ns, true},          /* ns */
    {Boot2KnownTitleId::lm, true},          /* lm */
    {Boot2KnownTitleId::ppc, true},         /* ppc */
    {Boot2KnownTitleId::ptm, true},         /* ptm */
    {Boot2KnownTitleId::hid, true},         /* hid */
    {Boot2KnownTitleId::audio, true},       /* audio */
    {Boot2KnownTitleId::lbl, true},         /* lbl */
    {Boot2KnownTitleId::wlan, true},        /* wlan */
    {Boot2KnownTitleId::bluetooth, true},   /* bluetooth */
    {Boot2KnownTitleId::bsdsockets, true},  /* bsdsockets */
    {Boot2KnownTitleId::nifm, true},        /* nifm */
    {Boot2KnownTitleId::ldn, true},         /* ldn */
    {Boot2KnownTitleId::account, true},     /* account */
    {Boot2KnownTitleId::friends, false},    /* friends */
    {Boot2KnownTitleId::nfc, true},         /* nfc */
    {Boot2KnownTitleId::jpegdec, true},     /* jpegdec */
    {Boot2KnownTitleId::capsrv, true},      /* capsrv */
    {Boot2KnownTitleId::ssl, true},         /* ssl */
    {Boot2KnownTitleId::nim, true},         /* nim */
    {Boot2KnownTitleId::bcat, false},       /* bcat */
    {Boot2KnownTitleId::erpt, true},        /* erpt */
    {Boot2KnownTitleId::es, true},          /* es */
    {Boot2KnownTitleId::pctl, true},        /* pctl */
    {Boot2KnownTitleId::btm, true},         /* btm */
    {Boot2KnownTitleId::eupld, false},      /* eupld */
    {Boot2KnownTitleId::glue, true},        /* glue */
    {Boot2KnownTitleId::eclct, true},       /* eclct */
    {Boot2KnownTitleId::npns, false},       /* npns */
    {Boot2KnownTitleId::fatal, true},       /* fatal */
    {Boot2KnownTitleId::ro, true},          /* ro */
    {Boot2KnownTitleId::profiler, true},    /* profiler */
    {Boot2KnownTitleId::sdb, true},         /* sdb */
    {Boot2KnownTitleId::migration, true},   /* migration */
    {Boot2KnownTitleId::grc, true},         /* grc */
};

void EmbeddedBoot2::Main() {     
    /* TODO: Modify initial program launch order, to get SD card mounted faster. */ 
    
    /* Launch psc. */
    LaunchTitle(Boot2KnownTitleId::psc, FsStorageId_NandSystem, 0, NULL);
    /* Launch settings. */
    LaunchTitle(Boot2KnownTitleId::settings, FsStorageId_NandSystem, 0, NULL);
    /* Launch usb. */
    LaunchTitle(Boot2KnownTitleId::usb, FsStorageId_NandSystem, 0, NULL);
    /* Launch pcie. */
    LaunchTitle(Boot2KnownTitleId::pcie, FsStorageId_NandSystem, 0, NULL);
    /* Launch bus. */
    LaunchTitle(Boot2KnownTitleId::bus, FsStorageId_NandSystem, 0, NULL);
    /* Launch tma. */
    LaunchTitle(Boot2KnownTitleId::tma, FsStorageId_NandSystem, 0, NULL);
    /* Launch pcv. */
    LaunchTitle(Boot2KnownTitleId::pcv, FsStorageId_NandSystem, 0, NULL);
    
    bool maintenance = ShouldForceMaintenanceMode();
    for (auto &launch_program : g_additional_launch_programs) {
        if (!maintenance || std::get<bool>(launch_program)) {
            LaunchTitle(std::get<Boot2KnownTitleId>(launch_program), FsStorageId_NandSystem, 0, NULL);
        }
    }
}
