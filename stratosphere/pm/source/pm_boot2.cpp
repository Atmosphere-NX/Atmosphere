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

static std::vector<u64> g_launched_titles;

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

static bool HasLaunchedTitle(u64 title_id) {
    return std::find(g_launched_titles.begin(), g_launched_titles.end(), title_id) != g_launched_titles.end();
}

static void SetLaunchedTitle(u64 title_id) {
    g_launched_titles.push_back(title_id);
}

static void ClearLaunchedTitles() {
    g_launched_titles.clear();
}

static void LaunchTitle(u64 title_id, FsStorageId storage_id, u32 launch_flags, u64 *pid) {
    u64 local_pid = 0;

    /* Don't launch a title twice during boot2. */
    if (HasLaunchedTitle(title_id)) {
        return;
    }

    Result rc = Registration::LaunchProcessByTidSid(Registration::TidSid{title_id, storage_id}, launch_flags, &local_pid);
    switch (rc) {
        case ResultKernelResourceExhausted:
            /* Out of resource! */
            std::abort();
            break;
        case ResultKernelOutOfMemory:
            /* Out of memory! */
            std::abort();
            break;
        case ResultKernelLimitReached:
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
    Result rc;
    DoWithSmSession([&]() {
        rc = setsysInitialize();
    });
    if (R_SUCCEEDED(rc)) {
        ON_SCOPE_EXIT { setsysExit(); };

        u8 force_maintenance = 1;
        setsysGetSettingsItemValue("boot", "force_maintenance", &force_maintenance, sizeof(force_maintenance));
        if (force_maintenance != 0) {
            return true;
        }
    }

    /* Contact GPIO, read plus/minus buttons. */
    DoWithSmSession([&]() {
        rc = gpioInitialize();
    });
    if (R_SUCCEEDED(rc)) {
        ON_SCOPE_EXIT { gpioExit(); };

        return GetGpioPadLow(GpioPadName_ButtonVolUp) && GetGpioPadLow(GpioPadName_ButtonVolDown);
    }

    return false;
}

static const std::tuple<u64, bool> g_additional_launch_programs[] = {
    {TitleId_Am, true},          /* am */
    {TitleId_NvServices, true},  /* nvservices */
    {TitleId_NvnFlinger, true},  /* nvnflinger */
    {TitleId_Vi, true},          /* vi */
    {TitleId_Ns, true},          /* ns */
    {TitleId_LogManager, true},  /* lm */
    {TitleId_Ppc, true},         /* ppc */
    {TitleId_Ptm, true},         /* ptm */
    {TitleId_Hid, true},         /* hid */
    {TitleId_Audio, true},       /* audio */
    {TitleId_Lbl, true},         /* lbl */
    {TitleId_Wlan, true},        /* wlan */
    {TitleId_Bluetooth, true},   /* bluetooth */
    {TitleId_BsdSockets, true},  /* bsdsockets */
    {TitleId_Nifm, true},        /* nifm */
    {TitleId_Ldn, true},         /* ldn */
    {TitleId_Account, true},     /* account */
    {TitleId_Friends, false},    /* friends */
    {TitleId_Nfc, true},         /* nfc */
    {TitleId_JpegDec, true},     /* jpegdec */
    {TitleId_CapSrv, true},      /* capsrv */
    {TitleId_Ssl, true},         /* ssl */
    {TitleId_Nim, true},         /* nim */
    {TitleId_Bcat, false},       /* bcat */
    {TitleId_Erpt, true},        /* erpt */
    {TitleId_Es, true},          /* es */
    {TitleId_Pctl, true},        /* pctl */
    {TitleId_Btm, true},         /* btm */
    {TitleId_Eupld, false},      /* eupld */
    {TitleId_Glue, true},        /* glue */
 /* {TitleId_Eclct, true}, */    /* eclct */      /* Skip launching error collection in Atmosphere to lessen telemetry. */
    {TitleId_Npns, false},       /* npns */
    {TitleId_Fatal, true},       /* fatal */
    {TitleId_Ro, true},          /* ro */
    {TitleId_Profiler, true},    /* profiler */
    {TitleId_Sdb, true},         /* sdb */
    {TitleId_Migration, true},   /* migration */
    {TitleId_Grc, true},         /* grc */
    {TitleId_Olsc, true},        /* olsc */
};

static void MountSdCard() {
    DoWithSmSession([&]() {
        Handle tmp_hnd = 0;
        static const char * const required_active_services[] = {"pcv", "gpio", "pinmux", "psc:c"};
        for (unsigned int i = 0; i < sizeof(required_active_services) / sizeof(required_active_services[0]); i++) {
            if (R_FAILED(smGetServiceOriginal(&tmp_hnd, smEncodeName(required_active_services[i])))) {
                /* TODO: Panic */
            } else {
                svcCloseHandle(tmp_hnd);
            }
        }
    });
    fsdevMountSdmc();
}

static void WaitForMitm(const char *service) {
    bool mitm_installed = false;

    Result rc;
    DoWithSmSession([&]() {
        if (R_FAILED((rc = smManagerAmsInitialize()))) {
            std::abort();
        }
    });

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
    LaunchTitle(TitleId_Psc, FsStorageId_NandSystem, 0, NULL);
    /* Launch pcie. */
    LaunchTitle(TitleId_Pcie, FsStorageId_NandSystem, 0, NULL);
    /* Launch bus. */
    LaunchTitle(TitleId_Bus, FsStorageId_NandSystem, 0, NULL);
    /* Launch settings. */
    LaunchTitle(TitleId_Settings, FsStorageId_NandSystem, 0, NULL);
    /* Launch pcv. */
    LaunchTitle(TitleId_Pcv, FsStorageId_NandSystem, 0, NULL);

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
    LaunchTitle(TitleId_Usb, FsStorageId_NandSystem, 0, NULL);

    /* Launch tma. */
    LaunchTitle(TitleId_Tma, FsStorageId_NandSystem, 0, NULL);

    /* Launch Atmosphere dmnt, using FsStorageId_None to force SD card boot. */
    LaunchTitle(TitleId_Dmnt, FsStorageId_None, 0, NULL);

    /* Launch default programs. */
    for (auto &launch_program : g_additional_launch_programs) {
        if (!maintenance || std::get<bool>(launch_program)) {
            LaunchTitle(std::get<u64>(launch_program), FsStorageId_NandSystem, 0, NULL);
        }

        /* In 7.0.0, Npns was added to the list of titles to launch during maintenance. */
        if (maintenance && std::get<u64>(launch_program) == TitleId_Npns && GetRuntimeFirmwareVersion() >= FirmwareVersion_700) {
            LaunchTitle(TitleId_Npns, FsStorageId_NandSystem, 0, NULL);
        }
    }

    /* Allow for user-customizable programs. */
    DIR *titles_dir = opendir("sdmc:/atmosphere/titles");
    struct dirent *ent;
    if (titles_dir != NULL) {
        while ((ent = readdir(titles_dir)) != NULL) {
            if (strlen(ent->d_name) == 0x10 && IsHexadecimal(ent->d_name)) {
                u64 title_id = (u64)strtoul(ent->d_name, NULL, 16);
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

    /* We no longer need the SD card. */
    fsdevUnmountAll();

    /* Free the memory used to track what boot2 launches. */
    ClearLaunchedTitles();
}
