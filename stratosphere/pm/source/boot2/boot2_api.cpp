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

#include <cctype>
#include <dirent.h>
#include <stratosphere/cfg.hpp>
#include <stratosphere/ldr.hpp>
#include <stratosphere/pm.hpp>
#include "boot2_api.hpp"

namespace sts::boot2 {

    namespace {

        /* Launch lists. */

        /* psc, bus, pcv is the minimal set of required titles to get SD card. */
        /* bus depends on pcie, and pcv depends on settings. */
        constexpr ncm::TitleId PreSdCardLaunchPrograms[] = {
            ncm::TitleId{TitleId_Psc},         /* psc */
            ncm::TitleId{TitleId_Pcie},        /* pcie */
            ncm::TitleId{TitleId_Bus},         /* bus */
            ncm::TitleId{TitleId_Settings},    /* settings */
            ncm::TitleId{TitleId_Pcv},         /* pcv */
        };
        constexpr size_t NumPreSdCardLaunchPrograms = util::size(PreSdCardLaunchPrograms);

        constexpr ncm::TitleId AdditionalLaunchPrograms[] = {
            ncm::TitleId{TitleId_Usb},         /* usb */
            ncm::TitleId{TitleId_Tma},         /* tma */
            ncm::TitleId{TitleId_Am},          /* am */
            ncm::TitleId{TitleId_NvServices},  /* nvservices */
            ncm::TitleId{TitleId_NvnFlinger},  /* nvnflinger */
            ncm::TitleId{TitleId_Vi},          /* vi */
            ncm::TitleId{TitleId_Ns},          /* ns */
            ncm::TitleId{TitleId_LogManager},  /* lm */
            ncm::TitleId{TitleId_Ppc},         /* ppc */
            ncm::TitleId{TitleId_Ptm},         /* ptm */
            ncm::TitleId{TitleId_Hid},         /* hid */
            ncm::TitleId{TitleId_Audio},       /* audio */
            ncm::TitleId{TitleId_Lbl},         /* lbl */
            ncm::TitleId{TitleId_Wlan},        /* wlan */
            ncm::TitleId{TitleId_Bluetooth},   /* bluetooth */
            ncm::TitleId{TitleId_BsdSockets},  /* bsdsockets */
            ncm::TitleId{TitleId_Nifm},        /* nifm */
            ncm::TitleId{TitleId_Ldn},         /* ldn */
            ncm::TitleId{TitleId_Account},     /* account */
            ncm::TitleId{TitleId_Friends},     /* friends */
            ncm::TitleId{TitleId_Nfc},         /* nfc */
            ncm::TitleId{TitleId_JpegDec},     /* jpegdec */
            ncm::TitleId{TitleId_CapSrv},      /* capsrv */
            ncm::TitleId{TitleId_Ssl},         /* ssl */
            ncm::TitleId{TitleId_Nim},         /* nim */
            ncm::TitleId{TitleId_Bcat},        /* bcat */
            ncm::TitleId{TitleId_Erpt},        /* erpt */
            ncm::TitleId{TitleId_Es},          /* es */
            ncm::TitleId{TitleId_Pctl},        /* pctl */
            ncm::TitleId{TitleId_Btm},         /* btm */
            ncm::TitleId{TitleId_Eupld},       /* eupld */
            ncm::TitleId{TitleId_Glue},        /* glue */
         /* ncm::TitleId{TitleId_Eclct}, */    /* eclct */      /* Skip launching error collection in Atmosphere to lessen telemetry. */
            ncm::TitleId{TitleId_Npns},        /* npns */
            ncm::TitleId{TitleId_Fatal},       /* fatal */
            ncm::TitleId{TitleId_Ro},          /* ro */
            ncm::TitleId{TitleId_Profiler},    /* profiler */
            ncm::TitleId{TitleId_Sdb},         /* sdb */
            ncm::TitleId{TitleId_Migration},   /* migration */
            ncm::TitleId{TitleId_Grc},         /* grc */
            ncm::TitleId{TitleId_Olsc},        /* olsc */
        };
        constexpr size_t NumAdditionalLaunchPrograms = util::size(AdditionalLaunchPrograms);

        constexpr ncm::TitleId AdditionalMaintenanceLaunchPrograms[] = {
            ncm::TitleId{TitleId_Usb},         /* usb */
            ncm::TitleId{TitleId_Tma},         /* tma */
            ncm::TitleId{TitleId_Am},          /* am */
            ncm::TitleId{TitleId_NvServices},  /* nvservices */
            ncm::TitleId{TitleId_NvnFlinger},  /* nvnflinger */
            ncm::TitleId{TitleId_Vi},          /* vi */
            ncm::TitleId{TitleId_Ns},          /* ns */
            ncm::TitleId{TitleId_LogManager},  /* lm */
            ncm::TitleId{TitleId_Ppc},         /* ppc */
            ncm::TitleId{TitleId_Ptm},         /* ptm */
            ncm::TitleId{TitleId_Hid},         /* hid */
            ncm::TitleId{TitleId_Audio},       /* audio */
            ncm::TitleId{TitleId_Lbl},         /* lbl */
            ncm::TitleId{TitleId_Wlan},        /* wlan */
            ncm::TitleId{TitleId_Bluetooth},   /* bluetooth */
            ncm::TitleId{TitleId_BsdSockets},  /* bsdsockets */
            ncm::TitleId{TitleId_Nifm},        /* nifm */
            ncm::TitleId{TitleId_Ldn},         /* ldn */
            ncm::TitleId{TitleId_Account},     /* account */
            ncm::TitleId{TitleId_Nfc},         /* nfc */
            ncm::TitleId{TitleId_JpegDec},     /* jpegdec */
            ncm::TitleId{TitleId_CapSrv},      /* capsrv */
            ncm::TitleId{TitleId_Ssl},         /* ssl */
            ncm::TitleId{TitleId_Nim},         /* nim */
            ncm::TitleId{TitleId_Erpt},        /* erpt */
            ncm::TitleId{TitleId_Es},          /* es */
            ncm::TitleId{TitleId_Pctl},        /* pctl */
            ncm::TitleId{TitleId_Btm},         /* btm */
            ncm::TitleId{TitleId_Glue},        /* glue */
         /* ncm::TitleId{TitleId_Eclct}, */    /* eclct */      /* Skip launching error collection in Atmosphere to lessen telemetry. */
            ncm::TitleId{TitleId_Fatal},       /* fatal */
            ncm::TitleId{TitleId_Ro},          /* ro */
            ncm::TitleId{TitleId_Profiler},    /* profiler */
            ncm::TitleId{TitleId_Sdb},         /* sdb */
            ncm::TitleId{TitleId_Migration},   /* migration */
            ncm::TitleId{TitleId_Grc},         /* grc */
            ncm::TitleId{TitleId_Olsc},        /* olsc */
        };
        constexpr size_t NumAdditionalMaintenanceLaunchPrograms = util::size(AdditionalMaintenanceLaunchPrograms);

        /* Helpers. */
        inline bool IsHexadecimal(const char *str) {
            while (*str) {
                if (!std::isxdigit(static_cast<unsigned char>(*(str++)))) {
                    return false;
                }
            }
            return true;
        }

        void LaunchTitle(u64 *out_process_id, const ncm::TitleLocation &loc, u32 launch_flags) {
            u64 process_id = 0;

            /* Don't launch a title twice during boot2. */
            if (pm::info::HasLaunchedTitle(loc.title_id)) {
                return;
            }

            switch (pm::shell::LaunchTitle(&process_id, loc, launch_flags)) {
                case ResultKernelResourceExhausted:
                    /* Out of resource! */
                    std::abort();
                case ResultKernelOutOfMemory:
                    /* Out of memory! */
                    std::abort();
                case ResultKernelLimitReached:
                    /* Limit Reached! */
                    std::abort();
                default:
                    /* We don't care about other issues. */
                    break;
            }

            if (out_process_id) {
                *out_process_id = process_id;
            }
        }

        void LaunchList(const ncm::TitleId *launch_list, size_t num_entries) {
            for (size_t i = 0; i < num_entries; i++) {
                LaunchTitle(nullptr, ncm::MakeTitleLocation(launch_list[i], ncm::StorageId::NandSystem), 0);
            }
        }

        bool GetGpioPadLow(GpioPadName pad) {
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

        bool IsMaintenanceMode() {
            /* Contact set:sys, retrieve boot!force_maintenance. */
            DoWithSmSession([&]() {
                R_ASSERT(setsysInitialize());
            });
            {
                ON_SCOPE_EXIT { setsysExit(); };

                u8 force_maintenance = 1;
                setsysGetSettingsItemValue("boot", "force_maintenance", &force_maintenance, sizeof(force_maintenance));
                if (force_maintenance != 0) {
                    return true;
                }
            }

            /* Contact GPIO, read plus/minus buttons. */
            DoWithSmSession([&]() {
                R_ASSERT(gpioInitialize());
            });
            {
                ON_SCOPE_EXIT { gpioExit(); };

                return GetGpioPadLow(GpioPadName_ButtonVolUp) && GetGpioPadLow(GpioPadName_ButtonVolDown);
            }
        }

        void WaitForMitm(const char *service) {
            const auto name = sts::sm::ServiceName::Encode(service);

            while (true) {
                bool mitm_installed = false;
                R_ASSERT(sts::sm::mitm::HasMitm(&mitm_installed, name));
                if (mitm_installed) {
                    break;
                }
                svcSleepThread(1'000'000ull);
            }
        }

        void LaunchFlaggedProgramsFromSdCard() {
            /* Allow for user-customizable programs. */
            DIR *titles_dir = opendir("sdmc:/atmosphere/titles");
            struct dirent *ent;
            if (titles_dir != NULL) {
                while ((ent = readdir(titles_dir)) != NULL) {
                    if (strlen(ent->d_name) == 2 * sizeof(u64) && IsHexadecimal(ent->d_name)) {
                        ncm::TitleId title_id{strtoul(ent->d_name, NULL, 16)};
                        if (pm::info::HasLaunchedTitle(title_id)) {
                            return;
                        }
                        char title_path[FS_MAX_PATH];
                        std::snprintf(title_path, sizeof(title_path), "sdmc:/atmosphere/titles/%s/flags/boot2.flag", ent->d_name);
                        FILE *f_flag = fopen(title_path, "rb");
                        if (f_flag != NULL) {
                            fclose(f_flag);
                            LaunchTitle(nullptr, ncm::MakeTitleLocation(title_id, ncm::StorageId::None), 0);
                        }
                    }
                }
                closedir(titles_dir);
            }
        }

    }

    /* Boot2 API. */
    void LaunchBootPrograms() {
        /* Wait until fs.mitm has installed itself. We want this to happen as early as possible. */
        WaitForMitm("fsp-srv");

        /* Launch programs required to mount the SD card. */
        LaunchList(PreSdCardLaunchPrograms, NumPreSdCardLaunchPrograms);

        /* At this point, the SD card can be mounted. */
        cfg::WaitSdCardInitialized();
        R_ASSERT(fsdevMountSdmc());

        /* Find out whether we are maintenance mode. */
        const bool maintenance = IsMaintenanceMode();
        if (maintenance) {
            pm::bm::SetMaintenanceBoot();
        }

        /* Wait for other atmosphere mitm modules to initialize. */
        if (GetRuntimeFirmwareVersion() >= FirmwareVersion_200) {
            WaitForMitm("bpc");
        } else {
            WaitForMitm("bpc:c");
        }

        /* Launch Atmosphere dmnt, using FsStorageId_None to force SD card boot. */
        LaunchTitle(nullptr, ncm::MakeTitleLocation(ncm::TitleId{TitleId_Dmnt}, ncm::StorageId::None), 0);

        /* Launch additional programs. */
        if (maintenance) {
            LaunchList(AdditionalMaintenanceLaunchPrograms, NumAdditionalMaintenanceLaunchPrograms);
        } else {
            LaunchList(AdditionalLaunchPrograms, NumAdditionalLaunchPrograms);
        }

        /* Launch user programs off of the SD. */
        LaunchFlaggedProgramsFromSdCard();

        /* We no longer need the SD card. */
        fsdevUnmountAll();
    }

}
