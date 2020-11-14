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
#include "amsmitm_initialization.hpp"
#include "amsmitm_fs_utils.hpp"
#include "amsmitm_prodinfo_utils.hpp"
#include "bpc_mitm/bpc_ams_power_utils.hpp"
#include "set_mitm/settings_sd_kvs.hpp"

namespace ams::mitm {

    namespace {

        /* BIS key sources. */
        constexpr u8 BisKeySources[4][2][0x10] = {
            {
                {0xF8, 0x3F, 0x38, 0x6E, 0x2C, 0xD2, 0xCA, 0x32, 0xA8, 0x9A, 0xB9, 0xAA, 0x29, 0xBF, 0xC7, 0x48},
                {0x7D, 0x92, 0xB0, 0x3A, 0xA8, 0xBF, 0xDE, 0xE1, 0xA7, 0x4C, 0x3B, 0x6E, 0x35, 0xCB, 0x71, 0x06}
            },
            {
                {0x41, 0x00, 0x30, 0x49, 0xDD, 0xCC, 0xC0, 0x65, 0x64, 0x7A, 0x7E, 0xB4, 0x1E, 0xED, 0x9C, 0x5F},
                {0x44, 0x42, 0x4E, 0xDA, 0xB4, 0x9D, 0xFC, 0xD9, 0x87, 0x77, 0x24, 0x9A, 0xDC, 0x9F, 0x7C, 0xA4}
            },
            {
                {0x52, 0xC2, 0xE9, 0xEB, 0x09, 0xE3, 0xEE, 0x29, 0x32, 0xA1, 0x0C, 0x1F, 0xB6, 0xA0, 0x92, 0x6C},
                {0x4D, 0x12, 0xE1, 0x4B, 0x2A, 0x47, 0x4C, 0x1C, 0x09, 0xCB, 0x03, 0x59, 0xF0, 0x15, 0xF4, 0xE4}
            },
            {
                {0x52, 0xC2, 0xE9, 0xEB, 0x09, 0xE3, 0xEE, 0x29, 0x32, 0xA1, 0x0C, 0x1F, 0xB6, 0xA0, 0x92, 0x6C},
                {0x4D, 0x12, 0xE1, 0x4B, 0x2A, 0x47, 0x4C, 0x1C, 0x09, 0xCB, 0x03, 0x59, 0xF0, 0x15, 0xF4, 0xE4}
            }
        };

        constexpr u8 BisKekSource[0x10] = {
            0x34, 0xC1, 0xA0, 0xC4, 0x82, 0x58, 0xF8, 0xB4, 0xFA, 0x9E, 0x5E, 0x6A, 0xDA, 0xFC, 0x7E, 0x4F,
        };

        void InitializeThreadFunc(void *arg);

        constexpr size_t InitializeThreadStackSize = 0x4000;

        /* Globals. */
        os::Event g_init_event(os::EventClearMode_ManualClear);

        os::ThreadType g_initialize_thread;
        alignas(os::ThreadStackAlignment) u8 g_initialize_thread_stack[InitializeThreadStackSize];

        /* Console-unique data backup and protection. */
        FsFile g_bis_key_file;

        /* Emummc file protection. */
        FsFile g_emummc_file;

        /* Maintain exclusive access to the fusee-secondary archive. */
        FsFile g_secondary_file;
        FsFile g_sept_payload_file;

        constexpr inline bool IsHexadecimal(const char *str) {
            while (*str) {
                if (std::isxdigit(static_cast<unsigned char>(*str))) {
                    str++;
                } else {
                    return false;
                }
            }
            return true;
        }

        void GetBackupFileName(char *dst, size_t dst_size, const char *serial_number, const char *fn) {
            if (strlen(serial_number) > 0) {
                std::snprintf(dst, dst_size, "automatic_backups/%s_%s", serial_number, fn);
            } else {
                std::snprintf(dst, dst_size, "automatic_backups/%s", fn);
            }
        }

        void CreateAutomaticBackups() {
            /* Create a backup directory, if one doesn't exist. */
            mitm::fs::CreateAtmosphereSdDirectory("/automatic_backups");

            /* Initialize PRODINFO and get a reference for the device. */
            char device_reference[0x40] = {};
            ON_SCOPE_EXIT { std::memset(device_reference, 0, sizeof(device_reference)); };
            mitm::SaveProdInfoBackupsAndWipeMemory(device_reference, sizeof(device_reference));

            /* Backup BIS keys. */
            {
                u64 key_generation = 0;
                if (hos::GetVersion() >= hos::Version_5_0_0) {
                    R_ABORT_UNLESS(spl::GetConfig(std::addressof(key_generation), spl::ConfigItem::DeviceUniqueKeyGeneration));
                }

                u8 bis_keys[4][2][0x10];
                std::memset(bis_keys, 0xCC, sizeof(bis_keys));
                ON_SCOPE_EXIT { std::memset(bis_keys, 0xCC, sizeof(bis_keys)); };

                /* TODO: Clean this up. */
                for (size_t partition = 0; partition < 4; partition++) {
                    if (partition == 0) {
                        for (size_t i = 0; i < 2; i++) {
                            R_ABORT_UNLESS(spl::GenerateSpecificAesKey(bis_keys[partition][i], 0x10, BisKeySources[partition][i], 0x10, key_generation, i));
                        }
                    } else {
                        const u32 option = (partition == 3 && spl::IsRecoveryBoot()) ? 0x4 : 0x1;

                        spl::AccessKey access_key;
                        R_ABORT_UNLESS(spl::GenerateAesKek(std::addressof(access_key), BisKekSource, 0x10, key_generation, option));
                        for (size_t i = 0; i < 2; i++) {
                            R_ABORT_UNLESS(spl::GenerateAesKey(bis_keys[partition][i], 0x10, access_key, BisKeySources[partition][i], 0x10));
                        }
                    }
                }

                char bis_keys_backup_name[ams::fs::EntryNameLengthMax + 1];
                GetBackupFileName(bis_keys_backup_name, sizeof(bis_keys_backup_name), device_reference, "BISKEYS.bin");

                mitm::fs::CreateAtmosphereSdFile(bis_keys_backup_name, sizeof(bis_keys), ams::fs::CreateOption_None);
                R_ABORT_UNLESS(mitm::fs::OpenAtmosphereSdFile(&g_bis_key_file, bis_keys_backup_name, ams::fs::OpenMode_ReadWrite));
                R_ABORT_UNLESS(fsFileSetSize(&g_bis_key_file, sizeof(bis_keys)));
                R_ABORT_UNLESS(fsFileWrite(&g_bis_key_file, 0, bis_keys, sizeof(bis_keys), FsWriteOption_Flush));
                /* NOTE: g_bis_key_file is intentionally not closed here.  This prevents any other process from opening it. */
            }

            /* Open a reference to the fusee-secondary archive. */
            /* As upcoming/current atmosphere releases will contain more than one zip which users much choose between, */
            /* maintaining an open reference prevents cleanly the issue of "automatic" updaters selecting the incorrect */
            /* zip, and encourages good updating hygiene -- atmosphere should not be updated on SD while HOS is alive. */
            {
                R_ABORT_UNLESS(mitm::fs::OpenSdFile(std::addressof(g_secondary_file),    "/atmosphere/fusee-secondary.bin", ams::fs::OpenMode_Read));
                R_ABORT_UNLESS(mitm::fs::OpenSdFile(std::addressof(g_sept_payload_file), "/sept/payload.bin",               ams::fs::OpenMode_Read));
            }
        }

        /* Initialization implementation */
        void InitializeThreadFunc(void *arg) {
            /* Wait for the SD card to be ready. */
            cfg::WaitSdCardInitialized();

            /* Open global SD card file system, so that other threads can begin using the SD. */
            mitm::fs::OpenGlobalSdCardFileSystem();

            /* Mount the sd card at a convenient mountpoint. */
            ams::fs::MountSdCard(ams::fs::impl::SdCardFileSystemMountName);

            /* Initialize the reboot manager (load a payload off the SD). */
            /* Discard result, since it doesn't need to succeed. */
            mitm::bpc::LoadRebootPayload();

            /* Backup Calibration Binary and BIS keys. */
            CreateAutomaticBackups();

            /* If we're emummc, persist a write-handle to prevent other processes from touching the image. */
            if (emummc::IsActive()) {
                if (const char *emummc_file_path = emummc::GetFilePath(); emummc_file_path != nullptr) {
                    char emummc_path[ams::fs::EntryNameLengthMax + 1];
                    std::snprintf(emummc_path, sizeof(emummc_path), "%s/eMMC", emummc_file_path);
                    mitm::fs::OpenSdFile(&g_emummc_file, emummc_path, ams::fs::OpenMode_Read);
                }
            }

            /* Connect to set:sys. */
            sm::DoWithSession([]() {
                R_ABORT_UNLESS(setInitialize());
                R_ABORT_UNLESS(setsysInitialize());
            });

            /* Load settings off the SD card. */
            settings::fwdbg::InitializeSdCardKeyValueStore();

            /* Ensure that we reboot using the user's preferred method. */
            R_ABORT_UNLESS(mitm::bpc::DetectPreferredRebootFunctionality());

            /* Signal to waiters that we are ready. */
            g_init_event.Signal();
        }

    }

    void StartInitialize() {
        /* Initialize prodinfo. */
        mitm::InitializeProdInfoManagement();

        /* Launch initialize thread. */
        R_ABORT_UNLESS(os::CreateThread(std::addressof(g_initialize_thread), InitializeThreadFunc, nullptr, g_initialize_thread_stack, sizeof(g_initialize_thread_stack), AMS_GET_SYSTEM_THREAD_PRIORITY(mitm, InitializeThread)));
        os::SetThreadNamePointer(std::addressof(g_initialize_thread), AMS_GET_SYSTEM_THREAD_NAME(mitm, InitializeThread));
        os::StartThread(std::addressof(g_initialize_thread));
    }

    bool IsInitialized() {
        return g_init_event.TryWait();
    }

    void WaitInitialized() {
        g_init_event.Wait();
    }

}
