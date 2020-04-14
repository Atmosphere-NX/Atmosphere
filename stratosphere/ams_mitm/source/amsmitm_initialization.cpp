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
        constexpr int    InitializeThreadPriority  = -7;

        /* Globals. */
        os::Event g_init_event(os::EventClearMode_ManualClear);

        os::ThreadType g_initialize_thread;
        alignas(os::ThreadStackAlignment) u8 g_initialize_thread_stack[InitializeThreadStackSize];

        /* Console-unique data backup and protection. */
        constexpr size_t CalibrationBinarySize = 0x8000;
        u8 g_calibration_binary_storage_backup[CalibrationBinarySize];
        u8 g_calibration_binary_file_backup[CalibrationBinarySize];
        FsFile g_calibration_binary_file;
        FsFile g_bis_key_file;

        /* Emummc file protection. */
        FsFile g_emummc_file;

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

            /* Read the calibration binary. */
            {
                FsStorage calibration_binary_storage;
                R_ABORT_UNLESS(fsOpenBisStorage(&calibration_binary_storage, FsBisPartitionId_CalibrationBinary));
                ON_SCOPE_EXIT { fsStorageClose(&calibration_binary_storage); };

                R_ABORT_UNLESS(fsStorageRead(&calibration_binary_storage, 0, g_calibration_binary_storage_backup, CalibrationBinarySize));
            }

            /* Copy serial number from partition. */
            /* TODO: Define the magic numbers? Structs? */
            char serial_number[0x40] = {};
            std::memcpy(serial_number, g_calibration_binary_storage_backup + 0x250, 0x18);

            /* Backup the calibration binary. */
            {
                char calibration_binary_backup_name[ams::fs::EntryNameLengthMax + 1];
                GetBackupFileName(calibration_binary_backup_name, sizeof(calibration_binary_backup_name), serial_number, "PRODINFO.bin");

                mitm::fs::CreateAtmosphereSdFile(calibration_binary_backup_name, CalibrationBinarySize, ams::fs::CreateOption_None);
                R_ABORT_UNLESS(mitm::fs::OpenAtmosphereSdFile(&g_calibration_binary_file, calibration_binary_backup_name, ams::fs::OpenMode_ReadWrite));

                s64 file_size = 0;
                R_ABORT_UNLESS(fsFileGetSize(&g_calibration_binary_file, &file_size));

                bool is_file_backup_valid = file_size == CalibrationBinarySize;
                if (is_file_backup_valid) {
                    u64 read_size = 0;
                    R_ABORT_UNLESS(fsFileRead(&g_calibration_binary_file, 0, g_calibration_binary_file_backup, CalibrationBinarySize, FsReadOption_None, &read_size));
                    AMS_ABORT_UNLESS(read_size == CalibrationBinarySize);
                    is_file_backup_valid &= std::memcmp(g_calibration_binary_file_backup, "CAL0", 4) == 0;
                    is_file_backup_valid &= std::memcmp(g_calibration_binary_file_backup + 0x250, serial_number, 0x18) == 0;
                    const u32 cal_bin_size = *reinterpret_cast<const u32 *>(g_calibration_binary_file_backup + 0x8);
                    is_file_backup_valid &= cal_bin_size + 0x40 <= CalibrationBinarySize;
                    if (is_file_backup_valid) {
                        u8 calc_hash[SHA256_HASH_SIZE];
                        /* TODO: ams::crypto? */
                        sha256CalculateHash(calc_hash, g_calibration_binary_file_backup + 0x40, cal_bin_size);
                        is_file_backup_valid &= std::memcmp(calc_hash, g_calibration_binary_file_backup + 0x20, sizeof(calc_hash)) == 0;
                    }
                }

                if (!is_file_backup_valid) {
                    R_ABORT_UNLESS(fsFileSetSize(&g_calibration_binary_file, CalibrationBinarySize));
                    R_ABORT_UNLESS(fsFileWrite(&g_calibration_binary_file, 0, g_calibration_binary_storage_backup, CalibrationBinarySize, FsWriteOption_Flush));
                }

                /* Note: g_calibration_binary_file is intentionally not closed here. This prevents any other process from opening it. */
                std::memset(g_calibration_binary_file_backup, 0, CalibrationBinarySize);
                std::memset(g_calibration_binary_storage_backup, 0, CalibrationBinarySize);
            }

            /* Backup BIS keys. */
            {
                u64 key_generation = 0;
                if (hos::GetVersion() >= hos::Version_5_0_0) {
                    R_ABORT_UNLESS(splGetConfig(SplConfigItem_NewKeyGeneration, &key_generation));
                }

                u8 bis_keys[4][2][0x10];
                std::memset(bis_keys, 0xCC, sizeof(bis_keys));

                /* TODO: Clean this up. */
                for (size_t partition = 0; partition < 4; partition++) {
                    if (partition == 0) {
                        for (size_t i = 0; i < 2; i++) {
                            R_ABORT_UNLESS(splFsGenerateSpecificAesKey(BisKeySources[partition][i], key_generation, i, bis_keys[partition][i]));
                        }
                    } else {
                        const u32 option = (partition == 3 && spl::IsRecoveryBoot()) ? 0x4 : 0x1;

                        u8 access_key[0x10];
                        R_ABORT_UNLESS(splCryptoGenerateAesKek(BisKekSource, key_generation, option, access_key));
                        for (size_t i = 0; i < 2; i++) {
                            R_ABORT_UNLESS(splCryptoGenerateAesKey(access_key, BisKeySources[partition][i], bis_keys[partition][i]));
                        }
                    }
                }

                char bis_keys_backup_name[ams::fs::EntryNameLengthMax + 1];
                GetBackupFileName(bis_keys_backup_name, sizeof(bis_keys_backup_name), serial_number, "BISKEYS.bin");

                mitm::fs::CreateAtmosphereSdFile(bis_keys_backup_name, sizeof(bis_keys), ams::fs::CreateOption_None);
                R_ABORT_UNLESS(mitm::fs::OpenAtmosphereSdFile(&g_bis_key_file, bis_keys_backup_name, ams::fs::OpenMode_ReadWrite));
                R_ABORT_UNLESS(fsFileSetSize(&g_bis_key_file, sizeof(bis_keys)));
                R_ABORT_UNLESS(fsFileWrite(&g_bis_key_file, 0, bis_keys, sizeof(bis_keys), FsWriteOption_Flush));
                /* NOTE: g_bis_key_file is intentionally not closed here.  This prevents any other process from opening it. */
            }
        }

        /* Initialization implementation */
        void InitializeThreadFunc(void *arg) {
            /* Wait for the SD card to be ready. */
            cfg::WaitSdCardInitialized();

            /* Open global SD card file system, so that other threads can begin using the SD. */
            mitm::fs::OpenGlobalSdCardFileSystem();

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
        R_ABORT_UNLESS(os::CreateThread(std::addressof(g_initialize_thread), InitializeThreadFunc, nullptr, g_initialize_thread_stack, sizeof(g_initialize_thread_stack), InitializeThreadPriority));
        os::StartThread(std::addressof(g_initialize_thread));
    }

    bool IsInitialized() {
        return g_init_event.TryWait();
    }

    void WaitInitialized() {
        g_init_event.Wait();
    }

}
