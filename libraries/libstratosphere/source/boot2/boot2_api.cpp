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

namespace ams::boot2 {

    namespace {

        /* Launch lists. */

        /* psc, bus, pcv is the minimal set of required programs to get SD card. */
        /* bus depends on pcie, and pcv depends on settings. */
        constexpr ncm::SystemProgramId PreSdCardLaunchPrograms[] = {
            ncm::SystemProgramId::Psc,         /* psc */
            ncm::SystemProgramId::Pcie,        /* pcie */
            ncm::SystemProgramId::Bus,         /* bus */
            ncm::SystemProgramId::Settings,    /* settings */
            ncm::SystemProgramId::Pcv,         /* pcv */
            ncm::SystemProgramId::Usb,         /* usb */
        };
        constexpr size_t NumPreSdCardLaunchPrograms = util::size(PreSdCardLaunchPrograms);

        constexpr ncm::SystemProgramId AdditionalLaunchPrograms[] = {
            ncm::SystemProgramId::Tma,         /* tma */
            ncm::SystemProgramId::Am,          /* am */
            ncm::SystemProgramId::NvServices,  /* nvservices */
            ncm::SystemProgramId::NvnFlinger,  /* nvnflinger */
            ncm::SystemProgramId::Vi,          /* vi */
            ncm::SystemProgramId::Pgl,         /* pgl */
            ncm::SystemProgramId::Ns,          /* ns */
            ncm::SystemProgramId::LogManager,  /* lm */
            ncm::SystemProgramId::Ppc,         /* ppc */
            ncm::SystemProgramId::Ptm,         /* ptm */
            ncm::SystemProgramId::Hid,         /* hid */
            ncm::SystemProgramId::Audio,       /* audio */
            ncm::SystemProgramId::Lbl,         /* lbl */
            ncm::SystemProgramId::Wlan,        /* wlan */
            ncm::SystemProgramId::Bluetooth,   /* bluetooth */
            ncm::SystemProgramId::BsdSockets,  /* bsdsockets */
            ncm::SystemProgramId::Nifm,        /* nifm */
            ncm::SystemProgramId::Ldn,         /* ldn */
            ncm::SystemProgramId::Account,     /* account */
            ncm::SystemProgramId::Friends,     /* friends */
            ncm::SystemProgramId::Nfc,         /* nfc */
            ncm::SystemProgramId::JpegDec,     /* jpegdec */
            ncm::SystemProgramId::CapSrv,      /* capsrv */
            ncm::SystemProgramId::Ssl,         /* ssl */
            ncm::SystemProgramId::Nim,         /* nim */
            ncm::SystemProgramId::Bcat,        /* bcat */
            ncm::SystemProgramId::Erpt,        /* erpt */
            ncm::SystemProgramId::Es,          /* es */
            ncm::SystemProgramId::Pctl,        /* pctl */
            ncm::SystemProgramId::Btm,         /* btm */
            ncm::SystemProgramId::Eupld,       /* eupld */
            ncm::SystemProgramId::Glue,        /* glue */
         /* ncm::SystemProgramId::Eclct, */    /* eclct */      /* Skip launching error collection in Atmosphere to lessen telemetry. */
            ncm::SystemProgramId::Npns,        /* npns */
            ncm::SystemProgramId::Fatal,       /* fatal */
            ncm::SystemProgramId::Ro,          /* ro */
            ncm::SystemProgramId::Profiler,    /* profiler */
            ncm::SystemProgramId::Sdb,         /* sdb */
            ncm::SystemProgramId::Migration,   /* migration */
            ncm::SystemProgramId::Grc,         /* grc */
            ncm::SystemProgramId::Olsc,        /* olsc */
            ncm::SystemProgramId::Ngct,        /* ngct */
        };
        constexpr size_t NumAdditionalLaunchPrograms = util::size(AdditionalLaunchPrograms);

        constexpr ncm::SystemProgramId AdditionalMaintenanceLaunchPrograms[] = {
            ncm::SystemProgramId::Tma,         /* tma */
            ncm::SystemProgramId::Am,          /* am */
            ncm::SystemProgramId::NvServices,  /* nvservices */
            ncm::SystemProgramId::NvnFlinger,  /* nvnflinger */
            ncm::SystemProgramId::Vi,          /* vi */
            ncm::SystemProgramId::Pgl,         /* pgl */
            ncm::SystemProgramId::Ns,          /* ns */
            ncm::SystemProgramId::LogManager,  /* lm */
            ncm::SystemProgramId::Ppc,         /* ppc */
            ncm::SystemProgramId::Ptm,         /* ptm */
            ncm::SystemProgramId::Hid,         /* hid */
            ncm::SystemProgramId::Audio,       /* audio */
            ncm::SystemProgramId::Lbl,         /* lbl */
            ncm::SystemProgramId::Wlan,        /* wlan */
            ncm::SystemProgramId::Bluetooth,   /* bluetooth */
            ncm::SystemProgramId::BsdSockets,  /* bsdsockets */
            ncm::SystemProgramId::Nifm,        /* nifm */
            ncm::SystemProgramId::Ldn,         /* ldn */
            ncm::SystemProgramId::Account,     /* account */
            ncm::SystemProgramId::Nfc,         /* nfc */
            ncm::SystemProgramId::JpegDec,     /* jpegdec */
            ncm::SystemProgramId::CapSrv,      /* capsrv */
            ncm::SystemProgramId::Ssl,         /* ssl */
            ncm::SystemProgramId::Nim,         /* nim */
            ncm::SystemProgramId::Erpt,        /* erpt */
            ncm::SystemProgramId::Es,          /* es */
            ncm::SystemProgramId::Pctl,        /* pctl */
            ncm::SystemProgramId::Btm,         /* btm */
            ncm::SystemProgramId::Glue,        /* glue */
         /* ncm::SystemProgramId::Eclct, */    /* eclct */      /* Skip launching error collection in Atmosphere to lessen telemetry. */
            ncm::SystemProgramId::Fatal,       /* fatal */
            ncm::SystemProgramId::Ro,          /* ro */
            ncm::SystemProgramId::Profiler,    /* profiler */
            ncm::SystemProgramId::Sdb,         /* sdb */
            ncm::SystemProgramId::Migration,   /* migration */
            ncm::SystemProgramId::Grc,         /* grc */
            ncm::SystemProgramId::Olsc,        /* olsc */
            ncm::SystemProgramId::Ngct,        /* ngct */
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

        inline bool IsAllowedLaunchProgram(const ncm::ProgramLocation &loc) {
            if (loc.program_id == ncm::SystemProgramId::Pgl) {
                return hos::GetVersion() >= hos::Version_10_0_0;
            }
            return true;
        }

        void LaunchProgram(os::ProcessId *out_process_id, const ncm::ProgramLocation &loc, u32 launch_flags) {
            os::ProcessId process_id = os::InvalidProcessId;

            /* Only launch the process if we're allowed to. */
            if (IsAllowedLaunchProgram(loc)) {
                /* Launch, lightly validate result. */
                {
                    const auto launch_result = pm::shell::LaunchProgram(&process_id, loc, launch_flags);
                    AMS_ABORT_UNLESS(!(svc::ResultOutOfResource::Includes(launch_result)));
                    AMS_ABORT_UNLESS(!(svc::ResultOutOfMemory::Includes(launch_result)));
                    AMS_ABORT_UNLESS(!(svc::ResultLimitReached::Includes(launch_result)));
                }

                if (out_process_id) {
                    *out_process_id = process_id;
                }
            }
        }

        void LaunchList(const ncm::SystemProgramId *launch_list, size_t num_entries) {
            for (size_t i = 0; i < num_entries; i++) {
                LaunchProgram(nullptr, ncm::ProgramLocation::Make(launch_list[i], ncm::StorageId::BuiltInSystem), 0);
            }
        }

        bool GetGpioPadLow(DeviceCode device_code) {
            gpio::GpioPadSession button;
            if (R_FAILED(gpio::OpenSession(std::addressof(button), device_code))) {
                return false;
            }

            /* Ensure we close even on early return. */
            ON_SCOPE_EXIT { gpio::CloseSession(std::addressof(button)); };

            /* Set direction input. */
            gpio::SetDirection(std::addressof(button), gpio::Direction_Input);

            return gpio::GetValue(std::addressof(button)) == gpio::GpioValue_Low;
        }

        bool IsForceMaintenance() {
            u8 force_maintenance = 1;
            settings::fwdbg::GetSettingsItemValue(&force_maintenance, sizeof(force_maintenance), "boot", "force_maintenance");
            return force_maintenance != 0;
        }

        bool IsMaintenanceMode() {
            /* Contact set:sys, retrieve boot!force_maintenance. */
            if (IsForceMaintenance()) {
                return true;
            }

            /* Contact GPIO, read plus/minus buttons. */
            {
                return GetGpioPadLow(gpio::DeviceCode_ButtonVolUp) && GetGpioPadLow(gpio::DeviceCode_ButtonVolDn);
            }
        }

        template<typename F>
        void IterateOverFlaggedProgramsOnSdCard(F f) {
            /* Validate that the contents directory exists. */
            fs::DirectoryHandle contents_dir;
            if (R_FAILED(fs::OpenDirectory(&contents_dir, "sdmc:/atmosphere/contents", fs::OpenDirectoryMode_Directory))) {
                return;
            }
            ON_SCOPE_EXIT { fs::CloseDirectory(contents_dir); };

            /* Iterate over entries in the contents directory */
            fs::DirectoryEntry entry;
            s64 count;
            while (R_SUCCEEDED(fs::ReadDirectory(&count, &entry, contents_dir, 1)) && count == 1) {
                /* Check that the subdirectory can be converted to a program id. */
                if (std::strlen(entry.name) == 2 * sizeof(ncm::ProgramId) && IsHexadecimal(entry.name)) {
                    /* Check if we've already launched the program. */
                    ncm::ProgramId program_id{std::strtoul(entry.name, nullptr, 16)};

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
                    char path[fs::EntryNameLengthMax];
                    std::snprintf(path, sizeof(path), "sdmc:/atmosphere/contents/%016lx/mitm.lst", static_cast<u64>(program_id));

                    fs::FileHandle f;
                    if (R_FAILED(fs::OpenFile(&f, path, fs::OpenMode_Read))) {
                        return;
                    }
                    ON_SCOPE_EXIT { fs::CloseFile(f); };

                    R_ABORT_UNLESS(fs::ReadFile(&mitm_list_size, f, 0, mitm_list, sizeof(mitm_list)));
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
                    AMS_ABORT_UNLESS(name_len <= sizeof(sm::ServiceName));

                    /* Declare the service. */
                    R_ABORT_UNLESS(sm::mitm::DeclareFutureMitm(sm::ServiceName::Encode(mitm_list + offset, name_len)));

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
        R_ABORT_UNLESS(sm::mitm::WaitMitm(sm::ServiceName::Encode("fsp-srv")));

        /* Launch programs required to mount the SD card. */
        /* psc, bus, pcv (and usb on newer firmwares) is the minimal set of required programs. */
        /* bus depends on pcie, and pcv depends on settings. */
        {
            /* Launch psc. */
            LaunchProgram(nullptr, ncm::ProgramLocation::Make(ncm::SystemProgramId::Psc, ncm::StorageId::BuiltInSystem), 0);

            /* Launch pcie. */
            LaunchProgram(nullptr, ncm::ProgramLocation::Make(ncm::SystemProgramId::Pcie, ncm::StorageId::BuiltInSystem), 0);

            /* Launch bus. */
            LaunchProgram(nullptr, ncm::ProgramLocation::Make(ncm::SystemProgramId::Bus, ncm::StorageId::BuiltInSystem), 0);

            /* Launch settings. */
            LaunchProgram(nullptr, ncm::ProgramLocation::Make(ncm::SystemProgramId::Settings, ncm::StorageId::BuiltInSystem), 0);

            /* NOTE: Here we work around a race condition in the boot process by ensuring that settings initializes its db. */
            {
                /* Connect to set:sys. */
                sm::ScopedServiceHolder<::setsysInitialize, ::setsysExit> setsys_holder;
                AMS_ABORT_UNLESS(setsys_holder);

                /* Retrieve setting from the database. */
                u8 force_maintenance = 0;
                settings::fwdbg::GetSettingsItemValue(&force_maintenance, sizeof(force_maintenance), "boot", "force_maintenance");
            }

            /* Launch pcv. */
            LaunchProgram(nullptr, ncm::ProgramLocation::Make(ncm::SystemProgramId::Pcv, ncm::StorageId::BuiltInSystem), 0);

            /* Launch usb. */
            LaunchProgram(nullptr, ncm::ProgramLocation::Make(ncm::SystemProgramId::Usb, ncm::StorageId::BuiltInSystem), 0);
        }

        /* Wait for the SD card required services to be ready. */
        cfg::WaitSdCardRequiredServicesReady();

        /* Wait for other atmosphere mitm modules to initialize. */
        R_ABORT_UNLESS(sm::mitm::WaitMitm(sm::ServiceName::Encode("set:sys")));
        if (spl::GetSocType() == spl::SocType_Erista) {
            if (hos::GetVersion() >= hos::Version_2_0_0) {
                R_ABORT_UNLESS(sm::mitm::WaitMitm(sm::ServiceName::Encode("bpc")));
            } else {
                R_ABORT_UNLESS(sm::mitm::WaitMitm(sm::ServiceName::Encode("bpc:c")));
            }
        }

        /* Launch Atmosphere boot2, using NcmStorageId_None to force SD card boot. */
        LaunchProgram(nullptr, ncm::ProgramLocation::Make(ncm::SystemProgramId::Boot2, ncm::StorageId::None), 0);
    }

    void LaunchPostSdCardBootPrograms() {
        /* This code is normally run by boot2. */

        /* Find out whether we are maintenance mode. */
        const bool maintenance = IsMaintenanceMode();
        if (maintenance) {
            pm::bm::SetMaintenanceBoot();
        }

        /* Launch Atmosphere dmnt, using NcmStorageId_None to force SD card boot. */
        LaunchProgram(nullptr, ncm::ProgramLocation::Make(ncm::SystemProgramId::Dmnt, ncm::StorageId::None), 0);

        /* Check for and forward declare non-atmosphere mitm modules. */
        DetectAndDeclareFutureMitms();

        /* Launch additional programs. */
        if (maintenance) {
            LaunchList(AdditionalMaintenanceLaunchPrograms, NumAdditionalMaintenanceLaunchPrograms);
            /* Starting in 7.0.0, npns is launched during maintenance boot. */
            if (hos::GetVersion() >= hos::Version_7_0_0) {
                LaunchProgram(nullptr, ncm::ProgramLocation::Make(ncm::SystemProgramId::Npns, ncm::StorageId::BuiltInSystem), 0);
            }
        } else {
            LaunchList(AdditionalLaunchPrograms, NumAdditionalLaunchPrograms);
        }

        /* Launch user programs off of the SD. */
        LaunchFlaggedProgramsOnSdCard();
    }

}
