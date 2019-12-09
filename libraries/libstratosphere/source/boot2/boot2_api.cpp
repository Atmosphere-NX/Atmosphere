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
#include <dirent.h>
#include <stratosphere.hpp>

namespace ams::boot2 {

    namespace {

        /* Launch lists. */

        /* psc, bus, pcv is the minimal set of required programs to get SD card. */
        /* bus depends on pcie, and pcv depends on settings. */
        constexpr ncm::ProgramId PreSdCardLaunchPrograms[] = {
            ncm::ProgramId::Psc,         /* psc */
            ncm::ProgramId::Pcie,        /* pcie */
            ncm::ProgramId::Bus,         /* bus */
            ncm::ProgramId::Settings,    /* settings */
            ncm::ProgramId::Pcv,         /* pcv */
            ncm::ProgramId::Usb,         /* usb */
        };
        constexpr size_t NumPreSdCardLaunchPrograms = util::size(PreSdCardLaunchPrograms);

        constexpr ncm::ProgramId AdditionalLaunchPrograms[] = {
            ncm::ProgramId::Tma,         /* tma */
            ncm::ProgramId::Am,          /* am */
            ncm::ProgramId::NvServices,  /* nvservices */
            ncm::ProgramId::NvnFlinger,  /* nvnflinger */
            ncm::ProgramId::Vi,          /* vi */
            ncm::ProgramId::Ns,          /* ns */
            ncm::ProgramId::LogManager,  /* lm */
            ncm::ProgramId::Ppc,         /* ppc */
            ncm::ProgramId::Ptm,         /* ptm */
            ncm::ProgramId::Hid,         /* hid */
            ncm::ProgramId::Audio,       /* audio */
            ncm::ProgramId::Lbl,         /* lbl */
            ncm::ProgramId::Wlan,        /* wlan */
            ncm::ProgramId::Bluetooth,   /* bluetooth */
            ncm::ProgramId::BsdSockets,  /* bsdsockets */
            ncm::ProgramId::Nifm,        /* nifm */
            ncm::ProgramId::Ldn,         /* ldn */
            ncm::ProgramId::Account,     /* account */
            ncm::ProgramId::Friends,     /* friends */
            ncm::ProgramId::Nfc,         /* nfc */
            ncm::ProgramId::JpegDec,     /* jpegdec */
            ncm::ProgramId::CapSrv,      /* capsrv */
            ncm::ProgramId::Ssl,         /* ssl */
            ncm::ProgramId::Nim,         /* nim */
            ncm::ProgramId::Bcat,        /* bcat */
            ncm::ProgramId::Erpt,        /* erpt */
            ncm::ProgramId::Es,          /* es */
            ncm::ProgramId::Pctl,        /* pctl */
            ncm::ProgramId::Btm,         /* btm */
            ncm::ProgramId::Eupld,       /* eupld */
            ncm::ProgramId::Glue,        /* glue */
         /* ncm::ProgramId::Eclct, */    /* eclct */      /* Skip launching error collection in Atmosphere to lessen telemetry. */
            ncm::ProgramId::Npns,        /* npns */
            ncm::ProgramId::Fatal,       /* fatal */
            ncm::ProgramId::Ro,          /* ro */
            ncm::ProgramId::Profiler,    /* profiler */
            ncm::ProgramId::Sdb,         /* sdb */
            ncm::ProgramId::Migration,   /* migration */
            ncm::ProgramId::Grc,         /* grc */
            ncm::ProgramId::Olsc,        /* olsc */
            ncm::ProgramId::Ngct,        /* ngct */
        };
        constexpr size_t NumAdditionalLaunchPrograms = util::size(AdditionalLaunchPrograms);

        constexpr ncm::ProgramId AdditionalMaintenanceLaunchPrograms[] = {
            ncm::ProgramId::Tma,         /* tma */
            ncm::ProgramId::Am,          /* am */
            ncm::ProgramId::NvServices,  /* nvservices */
            ncm::ProgramId::NvnFlinger,  /* nvnflinger */
            ncm::ProgramId::Vi,          /* vi */
            ncm::ProgramId::Ns,          /* ns */
            ncm::ProgramId::LogManager,  /* lm */
            ncm::ProgramId::Ppc,         /* ppc */
            ncm::ProgramId::Ptm,         /* ptm */
            ncm::ProgramId::Hid,         /* hid */
            ncm::ProgramId::Audio,       /* audio */
            ncm::ProgramId::Lbl,         /* lbl */
            ncm::ProgramId::Wlan,        /* wlan */
            ncm::ProgramId::Bluetooth,   /* bluetooth */
            ncm::ProgramId::BsdSockets,  /* bsdsockets */
            ncm::ProgramId::Nifm,        /* nifm */
            ncm::ProgramId::Ldn,         /* ldn */
            ncm::ProgramId::Account,     /* account */
            ncm::ProgramId::Nfc,         /* nfc */
            ncm::ProgramId::JpegDec,     /* jpegdec */
            ncm::ProgramId::CapSrv,      /* capsrv */
            ncm::ProgramId::Ssl,         /* ssl */
            ncm::ProgramId::Nim,         /* nim */
            ncm::ProgramId::Erpt,        /* erpt */
            ncm::ProgramId::Es,          /* es */
            ncm::ProgramId::Pctl,        /* pctl */
            ncm::ProgramId::Btm,         /* btm */
            ncm::ProgramId::Glue,        /* glue */
         /* ncm::ProgramId::Eclct, */    /* eclct */      /* Skip launching error collection in Atmosphere to lessen telemetry. */
            ncm::ProgramId::Fatal,       /* fatal */
            ncm::ProgramId::Ro,          /* ro */
            ncm::ProgramId::Profiler,    /* profiler */
            ncm::ProgramId::Sdb,         /* sdb */
            ncm::ProgramId::Migration,   /* migration */
            ncm::ProgramId::Grc,         /* grc */
            ncm::ProgramId::Olsc,        /* olsc */
            ncm::ProgramId::Ngct,        /* ngct */
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

        inline bool IsNewLine(char c) {
            return c == '\r' || c == '\n';
        }

        void LaunchProgram(os::ProcessId *out_process_id, const ncm::ProgramLocation &loc, u32 launch_flags) {
            os::ProcessId process_id = os::InvalidProcessId;

            /* Launch, lightly validate result. */
            {
                const auto launch_result = pm::shell::LaunchProgram(&process_id, loc, launch_flags);
                AMS_ASSERT(!(svc::ResultOutOfResource::Includes(launch_result)));
                AMS_ASSERT(!(svc::ResultOutOfMemory::Includes(launch_result)));
                AMS_ASSERT(!(svc::ResultLimitReached::Includes(launch_result)));
            }

            if (out_process_id) {
                *out_process_id = process_id;
            }
        }

        void LaunchList(const ncm::ProgramId *launch_list, size_t num_entries) {
            for (size_t i = 0; i < num_entries; i++) {
                LaunchProgram(nullptr, ncm::ProgramLocation::Make(launch_list[i], ncm::StorageId::BuiltInSystem), 0);
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
            {
                u8 force_maintenance = 1;
                settings::fwdbg::GetSettingsItemValue(&force_maintenance, sizeof(force_maintenance), "boot", "force_maintenance");
                if (force_maintenance != 0) {
                    return true;
                }
            }

            /* Contact GPIO, read plus/minus buttons. */
            {
                return GetGpioPadLow(GpioPadName_ButtonVolUp) && GetGpioPadLow(GpioPadName_ButtonVolDown);
            }
        }

        template<typename F>
        void IterateOverFlaggedProgramsOnSdCard(F f) {
            /* Validate that the contents directory exists. */
            DIR *contents_dir = opendir("sdmc:/atmosphere/contents");
            if (contents_dir == nullptr) {
                return;
            }
            ON_SCOPE_EXIT { closedir(contents_dir); };

            /* Iterate over entries in the contents directory */
            struct dirent *ent;
            while ((ent = readdir(contents_dir)) != nullptr) {
                /* Check that the subdirectory can be converted to a program id. */
                if (std::strlen(ent->d_name) == 2 * sizeof(ncm::ProgramId) && IsHexadecimal(ent->d_name)) {
                    /* Check if we've already launched the program. */
                    ncm::ProgramId program_id{std::strtoul(ent->d_name, nullptr, 16)};

                    /* Check if the program is flagged. */
                    if (!cfg::HasContentSpecificFlag(program_id, "boot2")) {
                        continue;
                    }

                    /* Call the iteration callback. */
                    f(program_id);
                }
            }
        }

        void DetectAndDeclareFutureMitms() {
            IterateOverFlaggedProgramsOnSdCard([](ncm::ProgramId program_id) {
                /* When we find a flagged program, check if it has a mitm list. */
                char mitm_list[0x400];
                size_t mitm_list_size = 0;

                /* Read the mitm list off the SD card. */
                {
                    char path[FS_MAX_PATH];
                    std::snprintf(mitm_list, sizeof(mitm_list), "sdmc:/atmosphere/contents/%016lx/mitm.lst", static_cast<u64>(program_id));
                    FILE *f = fopen(path, "rb");
                    if (f == nullptr) {
                        return;
                    }
                    mitm_list_size = static_cast<size_t>(fread(mitm_list, 1, sizeof(mitm_list), f));
                    fclose(f);
                }

                /* Validate read size. */
                if (mitm_list_size > sizeof(mitm_list) || mitm_list_size == 0) {
                    return;
                }

                /* Iterate over the contents of the file. */
                /* We expect one service name per non-empty line, 1-8 characters. */
                size_t offset = 0;
                while (offset < mitm_list_size) {
                    /* Continue to the start of the next name. */
                    while (IsNewLine(mitm_list[offset])) {
                        offset++;
                    }
                    if (offset >= mitm_list_size) {
                        break;
                    }

                    /* Get the length of the current name. */
                    size_t name_len;
                    for (name_len = 0; name_len <= sizeof(sm::ServiceName) && offset + name_len < mitm_list_size; name_len++) {
                        if (IsNewLine(mitm_list[offset + name_len])) {
                            break;
                        }
                    }

                    /* Allow empty lines. */
                    if (name_len == 0) {
                        continue;
                    }

                    /* Don't allow invalid lines. */
                    AMS_ASSERT(name_len <= sizeof(sm::ServiceName));

                    /* Declare the service. */
                    R_ASSERT(sm::mitm::DeclareFutureMitm(sm::ServiceName::Encode(mitm_list + offset, name_len)));

                    /* Advance to the next line. */
                    offset += name_len;
                }
            });
        }

        void LaunchFlaggedProgramsOnSdCard() {
            IterateOverFlaggedProgramsOnSdCard([](ncm::ProgramId program_id) {
                /* Check if we've already launched the program. */
                if (pm::info::HasLaunchedProgram(program_id)) {
                    return;
                }

                /* Launch the program. */
                LaunchProgram(nullptr, ncm::ProgramLocation::Make(program_id, ncm::StorageId::None), 0);
            });
        }

    }

    /* Boot2 API. */

    void LaunchPreSdCardBootProgramsAndBoot2() {
        /* This code is normally run by PM. */

        /* Wait until fs.mitm has installed itself. We want this to happen as early as possible. */
        R_ASSERT(sm::mitm::WaitMitm(sm::ServiceName::Encode("fsp-srv")));

        /* Launch programs required to mount the SD card. */
        LaunchList(PreSdCardLaunchPrograms, NumPreSdCardLaunchPrograms);

        /* Wait for the SD card required services to be ready. */
        cfg::WaitSdCardRequiredServicesReady();

        /* Wait for other atmosphere mitm modules to initialize. */
        R_ASSERT(sm::mitm::WaitMitm(sm::ServiceName::Encode("set:sys")));
        if (hos::GetVersion() >= hos::Version_200) {
            R_ASSERT(sm::mitm::WaitMitm(sm::ServiceName::Encode("bpc")));
        } else {
            R_ASSERT(sm::mitm::WaitMitm(sm::ServiceName::Encode("bpc:c")));
        }

        /* Launch Atmosphere boot2, using NcmStorageId_None to force SD card boot. */
        LaunchProgram(nullptr, ncm::ProgramLocation::Make(ncm::ProgramId::Boot2, ncm::StorageId::None), 0);
    }

    void LaunchPostSdCardBootPrograms() {
        /* This code is normally run by boot2. */

        /* Find out whether we are maintenance mode. */
        const bool maintenance = IsMaintenanceMode();
        if (maintenance) {
            pm::bm::SetMaintenanceBoot();
        }

        /* Launch Atmosphere dmnt, using NcmStorageId_None to force SD card boot. */
        LaunchProgram(nullptr, ncm::ProgramLocation::Make(ncm::ProgramId::Dmnt, ncm::StorageId::None), 0);

        /* Check for and forward declare non-atmosphere mitm modules. */
        DetectAndDeclareFutureMitms();

        /* Launch additional programs. */
        if (maintenance) {
            LaunchList(AdditionalMaintenanceLaunchPrograms, NumAdditionalMaintenanceLaunchPrograms);
            /* Starting in 7.0.0, npns is launched during maintenance boot. */
            if (hos::GetVersion() >= hos::Version_700) {
                LaunchProgram(nullptr, ncm::ProgramLocation::Make(ncm::ProgramId::Npns, ncm::StorageId::BuiltInSystem), 0);
            }
        } else {
            LaunchList(AdditionalLaunchPrograms, NumAdditionalLaunchPrograms);
        }

        /* Launch user programs off of the SD. */
        LaunchFlaggedProgramsOnSdCard();
    }

}
