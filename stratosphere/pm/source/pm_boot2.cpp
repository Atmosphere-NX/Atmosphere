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
 
#include <cstdlib>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <malloc.h>

#include <switch.h>
#include <stratosphere.hpp>
#include "pm_boot2.hpp"
#include "pm_registration.hpp"
#include "pm_boot_mode.hpp"

static std::vector<Boot2KnownTitleId> g_launched_titles;

static bool IsHexadecimal(const char *str) {
    while (*str) {
        if (isxdigit(*str)) {
            str++;
        } else {
            return false;
        }
    }
    return true;
}

static bool HasLaunchedTitle(Boot2KnownTitleId title_id) {
    return std::find(g_launched_titles.begin(), g_launched_titles.end(), title_id) != g_launched_titles.end();
}

static void SetLaunchedTitle(Boot2KnownTitleId title_id) {
    g_launched_titles.push_back(title_id);
}

static void ClearLaunchedTitles() {
    g_launched_titles.clear();
}

static void LaunchTitle(Boot2KnownTitleId title_id, FsStorageId storage_id, u32 launch_flags, u64 *pid) {
    u64 local_pid = 0;
    
    /* Don't launch a title twice during boot2. */
    if (HasLaunchedTitle(title_id)) {
        return;
    }
    
    Result rc = Registration::LaunchProcessByTidSid(Registration::TidSid{(u64)title_id, storage_id}, launch_flags, &local_pid);
    switch (rc) {
        case 0xCE01:
            /* Out of resource! */
            std::abort();
            break;
        case 0xDE01:
            /* Out of memory! */
            std::abort();
            break;
        case 0xD001:
            /* Limit Reached! */
            std::abort();
            break;
        default:
            /* We don't care about other issues. */
            break;
    }
    if (pid) {
        *pid = local_pid;
    }
    
    if (R_SUCCEEDED(rc)) {
        SetLaunchedTitle(title_id);
    }
}

static bool GetGpioPadLow(GpioPadName pad) {
    GpioPadSession button;
    if (R_FAILED(gpioOpenSession(&button, pad))) {
        return false;
    }
    
    /* Ensure we close even on early return. */
    ON_SCOPE_EXIT { gpioPadClose(&button); };
    
    /* Set direction input. */
    gpioPadSetDirection(&button, GpioDirection_Input);
    
    GpioValue val;
    return R_SUCCEEDED(gpioPadGetValue(&button, &val)) && val == GpioValue_Low;
}

static bool IsMaintenanceMode() {
    /* Contact set:sys, retrieve boot!force_maintenance. */
    if (R_SUCCEEDED(setsysInitialize())) {
        ON_SCOPE_EXIT { setsysExit(); };
        
        u8 force_maintenance = 1;
        setsysGetSettingsItemValue("boot", "force_maintenance", &force_maintenance, sizeof(force_maintenance));
        if (force_maintenance != 0) {
            return true;
        }
    }

    /* Contact GPIO, read plus/minus buttons. */
    if (R_SUCCEEDED(gpioInitialize())) {
        ON_SCOPE_EXIT { gpioExit(); };
        
        return GetGpioPadLow(GpioPadName_ButtonVolUp) && GetGpioPadLow(GpioPadName_ButtonVolDown);
    }
    
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
 /* {Boot2KnownTitleId::eclct, true}, */    /* eclct */      /* Skip launching error collection in Atmosphere to lessen telemetry. */
    {Boot2KnownTitleId::npns, false},       /* npns */
    {Boot2KnownTitleId::fatal, true},       /* fatal */
    {Boot2KnownTitleId::ro, true},          /* ro */
    {Boot2KnownTitleId::profiler, true},    /* profiler */
    {Boot2KnownTitleId::sdb, true},         /* sdb */
    {Boot2KnownTitleId::migration, true},   /* migration */
    {Boot2KnownTitleId::grc, true},         /* grc */
    {Boot2KnownTitleId::olsc, true},        /* olsc */
};

static void MountSdCard() {
    Handle tmp_hnd = 0;
    static const char * const required_active_services[] = {"pcv", "gpio", "pinmux", "psc:c"};
    for (unsigned int i = 0; i < sizeof(required_active_services) / sizeof(required_active_services[0]); i++) {
        if (R_FAILED(smGetServiceOriginal(&tmp_hnd, smEncodeName(required_active_services[i])))) {
            /* TODO: Panic */
        } else {
            svcCloseHandle(tmp_hnd);   
        }
    }
    fsdevMountSdmc();
}

static void WaitForMitm(const char *service) {
    bool mitm_installed = false;

    Result rc = smManagerAmsInitialize();
    if (R_FAILED(rc)) {
        std::abort();
    }
    while (R_FAILED((rc = smManagerAmsHasMitm(&mitm_installed, service))) || !mitm_installed) {
        if (R_FAILED(rc)) {
            std::abort();
        }
        svcSleepThread(1000000ull);
    }
    smManagerAmsExit();
}

void EmbeddedBoot2::Main() {
    /* Wait until fs.mitm has installed itself. We want this to happen as early as possible. */
    WaitForMitm("fsp-srv");
    
    /* Clear titles. */
    ClearLaunchedTitles();

    /* psc, bus, pcv is the minimal set of required titles to get SD card. */ 
    /* bus depends on pcie, and pcv depends on settings. */
    /* Launch psc. */
    LaunchTitle(Boot2KnownTitleId::psc, FsStorageId_NandSystem, 0, NULL);
    /* Launch pcie. */
    LaunchTitle(Boot2KnownTitleId::pcie, FsStorageId_NandSystem, 0, NULL);
    /* Launch bus. */
    LaunchTitle(Boot2KnownTitleId::bus, FsStorageId_NandSystem, 0, NULL);
    /* Launch settings. */
    LaunchTitle(Boot2KnownTitleId::settings, FsStorageId_NandSystem, 0, NULL);
    /* Launch pcv. */
    LaunchTitle(Boot2KnownTitleId::pcv, FsStorageId_NandSystem, 0, NULL);
    
    /* At this point, the SD card can be mounted. */
    MountSdCard();
    
    /* Find out whether we are maintenance mode. */
    bool maintenance = IsMaintenanceMode();
    if (maintenance) {
        BootModeService::SetMaintenanceBootForEmbeddedBoot2();
    }
        
    /* Wait for other atmosphere mitm modules to initialize. */
    WaitForMitm("set:sys");
    if (GetRuntimeFirmwareVersion() >= FirmwareVersion_200) {
        WaitForMitm("bpc");
    } else {
        WaitForMitm("bpc:c");
    }
    
    /* Launch usb. */
    LaunchTitle(Boot2KnownTitleId::usb, FsStorageId_NandSystem, 0, NULL);
    
    /* Launch tma. */
    LaunchTitle(Boot2KnownTitleId::tma, FsStorageId_NandSystem, 0, NULL);
      
    /* Launch Atmosphere dmnt, using FsStorageId_None to force SD card boot. */
    LaunchTitle(Boot2KnownTitleId::dmnt, FsStorageId_None, 0, NULL);
    
    /* Launch default programs. */
    for (auto &launch_program : g_additional_launch_programs) {
        if (!maintenance || std::get<bool>(launch_program)) {
            LaunchTitle(std::get<Boot2KnownTitleId>(launch_program), FsStorageId_NandSystem, 0, NULL);
        }
    }
    
    /* Allow for user-customizable programs. */
    DIR *titles_dir = opendir("sdmc:/atmosphere/titles");
    struct dirent *ent;
    if (titles_dir != NULL) {
        while ((ent = readdir(titles_dir)) != NULL) {
            if (strlen(ent->d_name) == 0x10 && IsHexadecimal(ent->d_name)) {
                Boot2KnownTitleId title_id = (Boot2KnownTitleId)strtoul(ent->d_name, NULL, 16);
                if (HasLaunchedTitle(title_id)) {
                    continue;
                }
                char title_path[FS_MAX_PATH] = {0};
                strcpy(title_path, "sdmc:/atmosphere/titles/");
                strcat(title_path, ent->d_name);
                strcat(title_path, "/flags/boot2.flag");
                FILE *f_flag = fopen(title_path, "rb");
                if (f_flag != NULL) {
                    fclose(f_flag);
                    LaunchTitle(title_id, FsStorageId_None, 0, NULL);
                } else {
                    /* TODO: Deprecate this in the future. */
                    memset(title_path, 0, FS_MAX_PATH);
                    strcpy(title_path, "sdmc:/atmosphere/titles/");
                    strcat(title_path, ent->d_name);
                    strcat(title_path, "/boot2.flag");
                    f_flag = fopen(title_path, "rb");
                    if (f_flag != NULL) {
                        fclose(f_flag);
                        LaunchTitle(title_id, FsStorageId_None, 0, NULL);
                    }
                }
            }
        }
        closedir(titles_dir);
    }
    
    /* Free the memory used to track what boot2 launches. */
    ClearLaunchedTitles();
        
    /* We no longer need the SD card. */
    fsdevUnmountAll();
    
    /* Clear titles. */
    ClearLaunchedTitles();
}
