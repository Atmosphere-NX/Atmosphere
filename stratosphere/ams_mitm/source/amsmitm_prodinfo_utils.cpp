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
#include "amsmitm_fs_utils.hpp"
#include "amsmitm_prodinfo_utils.hpp"

namespace ams::mitm {

    namespace {

        constexpr inline u16 Crc16InitialValue = 0x55AA;

        constexpr inline u16 Crc16Table[] = {
            0x0000, 0xCC01, 0xD801, 0x1400,
            0xF001, 0x3C00, 0x2800, 0xE401,
            0xA001, 0x6C00, 0x7800, 0xB401,
            0x5000, 0x9C01, 0x8801, 0x4400,
        };

        u16 GetCrc16(const void *data, size_t size) {
            AMS_ASSERT(data != nullptr);
            AMS_ASSERT(size > 0);

            const u8 *src = static_cast<const u8 *>(data);

            u16 crc = Crc16InitialValue;

            u16 tmp = 0;
            while ((size--) > 0) {
                tmp = Crc16Table[crc & 0xF];
                crc = ((crc >> 4) & 0x0FFF) ^ tmp ^ Crc16Table[*src & 0xF];
                tmp = Crc16Table[crc & 0xF];
                crc = ((crc >> 4) & 0x0FFF) ^ tmp ^ Crc16Table[(*(src++) >> 4) & 0xF];
            }
            return crc;
        }

        bool IsBlank(const void *data, size_t size) {
            AMS_ASSERT(data != nullptr);
            AMS_ASSERT(size > 0);

            const u8 *src = static_cast<const u8 *>(data);
            while ((size--) > 0) {
                if (*(src++) != 0) {
                    return false;
                }
            }
            return true;
        }

        constexpr inline u32 CalibrationMagic = util::FourCC<'C','A','L','0'>::Code;

        struct Sha256Hash {
            u8 data[crypto::Sha256Generator::HashSize];
        };

        struct CalibrationInfoHeader {
            u32 magic;
            u32 version;
            u32 body_size;
            u16 model;
            u16 update_count;
            u8  pad[0xE];
            u16 crc;
            Sha256Hash body_hash;
        };
        static_assert(sizeof(CalibrationInfoHeader) == 0x40);

        constexpr inline size_t CalibrationInfoBodySizeMax = CalibrationBinarySize - sizeof(CalibrationInfoHeader);

        struct CalibrationInfo {
            CalibrationInfoHeader header;
            u8 body[CalibrationInfoBodySizeMax];   /* TODO: CalibrationInfoBody body; */

            template<typename Block>
            Block &GetBlock() {
                static_assert(Block::Offset >= sizeof(CalibrationInfoHeader));
                static_assert(Block::Offset < sizeof(CalibrationInfo));
                static_assert(Block::Offset + Block::Size <= sizeof(CalibrationInfo));
                return *static_cast<Block *>(static_cast<void *>(std::addressof(this->body[Block::Offset - sizeof(this->header)])));
            }

            template<typename Block>
            const Block &GetBlock() const {
                static_assert(Block::Offset >= sizeof(CalibrationInfoHeader));
                static_assert(Block::Offset < sizeof(CalibrationInfo));
                static_assert(Block::Offset + Block::Size <= sizeof(CalibrationInfo));
                return *static_cast<const Block *>(static_cast<const void *>(std::addressof(this->body[Block::Offset - sizeof(this->header)])));
            }
        };
        static_assert(sizeof(CalibrationInfo) == CalibrationBinarySize);

        struct SecureCalibrationInfoBackup  {
            CalibrationInfo info;
            Sha256Hash hash;
            u8 pad[SecureCalibrationBinaryBackupSize - sizeof(info) - sizeof(hash)];
        };
        static_assert(sizeof(SecureCalibrationInfoBackup) == SecureCalibrationBinaryBackupSize);

        bool IsValidSha256Hash(const Sha256Hash &hash, const void *data, size_t data_size) {
            Sha256Hash calc_hash;
            ON_SCOPE_EXIT { ::ams::crypto::ClearMemory(std::addressof(calc_hash), sizeof(calc_hash)); };

            ::ams::crypto::GenerateSha256Hash(std::addressof(calc_hash), sizeof(calc_hash), data, data_size);
            return ::ams::crypto::IsSameBytes(std::addressof(calc_hash), std::addressof(hash), sizeof(Sha256Hash));
        }

        bool IsValid(const CalibrationInfoHeader &header) {
            return header.magic == CalibrationMagic && GetCrc16(std::addressof(header), OFFSETOF(CalibrationInfoHeader, crc)) == header.crc;
        }

        bool IsValid(const CalibrationInfoHeader &header, const void *body) {
            return IsValid(header) && IsValidSha256Hash(header.body_hash, body, header.body_size);
        }

        #define DEFINE_CALIBRATION_CRC_BLOCK(_TypeName, _Offset, _Size, _Decl, _MemberName) \
        struct _TypeName {                                                                  \
            static constexpr size_t Offset   = _Offset;                                     \
            static constexpr size_t Size     = _Size;                                       \
            static constexpr bool IsCrcBlock = true;                                        \
            static constexpr bool IsShaBlock = false;                                       \
            _Decl;                                                                          \
            static_assert(Size >= sizeof(_MemberName) + sizeof(u16));                       \
            u8 pad[Size - sizeof(_MemberName) - sizeof(u16)];                               \
            u16 crc;                                                                        \
        };                                                                                  \
        static_assert(sizeof(_TypeName) == _TypeName::Size)

        #define DEFINE_CALIBRATION_SHA_BLOCK(_TypeName, _Offset, _Size, _Decl, _MemberName) \
        struct _TypeName {                                                                  \
            static constexpr size_t Offset = _Offset;                                       \
            static constexpr size_t Size   = _Size;                                         \
            static constexpr bool IsCrcBlock = false;                                       \
            static constexpr bool IsShaBlock = true;                                        \
            _Decl;                                                                          \
            static_assert(Size == sizeof(_MemberName) + sizeof(Sha256Hash));                \
            Sha256Hash sha256_hash;                                                         \
        };                                                                                  \
        static_assert(sizeof(_TypeName) == _TypeName::Size)

        DEFINE_CALIBRATION_CRC_BLOCK(SerialNumberBlock,                     0x0250, 0x020, ::ams::settings::factory::SerialNumber serial_number, serial_number);
        DEFINE_CALIBRATION_CRC_BLOCK(EccB233DeviceCertificateBlock,         0x0480, 0x190, ::ams::settings::factory::EccB233DeviceCertificate device_certificate, device_certificate);
        DEFINE_CALIBRATION_CRC_BLOCK(SslKeyBlock,                           0x09B0, 0x120, u8 ssl_key[0x110], ssl_key);
        DEFINE_CALIBRATION_CRC_BLOCK(SslCertificateSizeBlock,               0x0AD0, 0x010, u64 ssl_certificate_size, ssl_certificate_size);
        DEFINE_CALIBRATION_SHA_BLOCK(SslCertificateBlock,                   0x0AE0, 0x820, u8 ssl_certificate[0x800], ssl_certificate);
        DEFINE_CALIBRATION_CRC_BLOCK(EcqvEcdsaAmiiboRootCertificateBlock,   0x35A0, 0x080, u8 data[0x70], data);
        DEFINE_CALIBRATION_CRC_BLOCK(EcqvBlsAmiiboRootCertificateBlock,     0x36A0, 0x0A0, u8 data[0x90], data);
        DEFINE_CALIBRATION_CRC_BLOCK(ExtendedSslKeyBlock,                   0x3AE0, 0x140, u8 ssl_key[0x134], ssl_key);
        DEFINE_CALIBRATION_CRC_BLOCK(Rsa2048DeviceKeyBlock,                 0x3D70, 0x250, u8 device_key[0x240], device_key);
        DEFINE_CALIBRATION_CRC_BLOCK(Rsa2048DeviceCertificateBlock,         0x3FC0, 0x250, ::ams::settings::factory::Rsa2048DeviceCertificate device_certificate, device_certificate);

        #undef DEFINE_CALIBRATION_CRC_BLOCK
        #undef DEFINE_CALIBRATION_SHA_BLOCK

        constexpr inline const char BlankSerialNumberString[] = "XAW00000000000";

        template<typename Block>
        void Blank(Block &block) {
            if constexpr (std::is_same<Block, SerialNumberBlock>::value) {
                static_assert(sizeof(BlankSerialNumberString) <= sizeof(SerialNumberBlock::serial_number));
                std::memset(std::addressof(block), 0, Block::Size - sizeof(block.crc));
                std::memcpy(block.serial_number.str, BlankSerialNumberString, sizeof(BlankSerialNumberString));
                block.crc = GetCrc16(std::addressof(block), Block::Size - sizeof(block.crc));
            } else if constexpr (std::is_same<Block, SslCertificateBlock>::value) {
                std::memset(std::addressof(block), 0, sizeof(block.ssl_certificate));
            } else if constexpr (Block::IsCrcBlock) {
                std::memset(std::addressof(block), 0, Block::Size - sizeof(block.crc));
                block.crc = GetCrc16(std::addressof(block), Block::Size - sizeof(block.crc));
            } else {
                static_assert(Block::IsShaBlock);
                std::memset(std::addressof(block), 0, Block::Size);
                ::ams::crypto::GenerateSha256Hash(std::addressof(block.sha256_hash), sizeof(block.sha256_hash), std::addressof(block), Block::Size - sizeof(block.sha256_hash));
            }
        }

        template<typename Block>
        bool IsBlank(const Block &block) {
            if constexpr (std::is_same<Block, SerialNumberBlock>::value) {
                static_assert(sizeof(BlankSerialNumberString) <= sizeof(SerialNumberBlock::serial_number));
                return std::memcmp(block.serial_number.str, BlankSerialNumberString, sizeof(BlankSerialNumberString) - 1) == 0 || IsBlank(std::addressof(block), Block::Size - sizeof(block.crc));
            } else if constexpr (Block::IsCrcBlock) {
                return IsBlank(std::addressof(block), Block::Size - sizeof(block.crc));
            } else {
                return IsBlank(std::addressof(block), Block::Size - sizeof(block.sha256_hash));
            }
        }

        template<typename Block>
        bool IsValid(const Block &block, size_t size = 0) {
            if constexpr (Block::IsCrcBlock) {
                return GetCrc16(std::addressof(block), Block::Size - sizeof(block.crc)) == block.crc;
            } else {
                static_assert(Block::IsShaBlock);
                return IsValidSha256Hash(block.sha256_hash, std::addressof(block), size != 0 ? size : Block::Size - sizeof(block.sha256_hash));
            }
        }


        void Blank(CalibrationInfo &info) {
            /* Set header. */
            info.header.magic       = CalibrationMagic;
            info.header.body_size   = sizeof(info.body);
            info.header.crc         = GetCrc16(std::addressof(info.header), OFFSETOF(CalibrationInfoHeader, crc));

            /* Set blocks. */
            Blank(info.GetBlock<SerialNumberBlock>());
            Blank(info.GetBlock<SslCertificateSizeBlock>());
            Blank(info.GetBlock<SslCertificateBlock>());
            Blank(info.GetBlock<EcqvEcdsaAmiiboRootCertificateBlock>());
            Blank(info.GetBlock<EcqvBlsAmiiboRootCertificateBlock>());
            Blank(info.GetBlock<ExtendedSslKeyBlock>());

            /* Set header hash. */
            crypto::GenerateSha256Hash(std::addressof(info.header.body_hash), sizeof(info.header.body_hash), std::addressof(info.body), sizeof(info.body));
        }

        bool IsValidHeader(const CalibrationInfo &cal) {
            return IsValid(cal.header) && cal.header.body_size <= CalibrationInfoBodySizeMax && IsValid(cal.header, cal.body);
        }

        bool IsValidSerialNumber(const char *sn) {
            for (size_t i = 0; i < std::strlen(sn); i++) {
                if (!std::isalnum(static_cast<unsigned char>(sn[i]))) {
                    return false;
                }
            }
            return true;
        }

        void GetSerialNumber(char *dst, const CalibrationInfo &info) {
            std::memcpy(dst, std::addressof(info.GetBlock<SerialNumberBlock>()), sizeof(info.GetBlock<SerialNumberBlock>().serial_number));
            dst[sizeof(info.GetBlock<SerialNumberBlock>().serial_number) + 1] = '\x00';
        }

        bool IsValidSerialNumber(const CalibrationInfo &cal) {
            char sn[0x20] = {};
            ON_SCOPE_EXIT { std::memset(sn, 0, sizeof(sn)); };

            GetSerialNumber(sn, cal);
            return IsValidSerialNumber(sn);
        }

        bool IsValid(const CalibrationInfo &cal) {
            return IsValidHeader(cal)                                                                                                          &&
                   IsValid(cal.GetBlock<SerialNumberBlock>())                                                                                  &&
                   IsValid(cal.GetBlock<EccB233DeviceCertificateBlock>())                                                                      &&
                   IsValid(cal.GetBlock<SslKeyBlock>())                                                                                        &&
                   IsValid(cal.GetBlock<SslCertificateSizeBlock>())                                                                            &&
                   cal.GetBlock<SslCertificateSizeBlock>().ssl_certificate_size <= sizeof(cal.GetBlock<SslCertificateBlock>().ssl_certificate) &&
                   IsValid(cal.GetBlock<SslCertificateBlock>(), cal.GetBlock<SslCertificateSizeBlock>().ssl_certificate_size)                  &&
                   IsValid(cal.GetBlock<EcqvEcdsaAmiiboRootCertificateBlock>())                                                                &&
                   IsValid(cal.GetBlock<EcqvBlsAmiiboRootCertificateBlock>())                                                                  &&
                   IsValid(cal.GetBlock<ExtendedSslKeyBlock>())                                                                                &&
                   IsValidSerialNumber(cal);
        }

        bool ContainsCorrectDeviceId(const EccB233DeviceCertificateBlock &block, u64 device_id) {
            static constexpr size_t DeviceIdOffset = 0xC6;
            char found_device_id_str[sizeof("0011223344556677")] = {};
            ON_SCOPE_EXIT { std::memset(found_device_id_str, 0, sizeof(found_device_id_str)); };
            std::memcpy(found_device_id_str, std::addressof(block.device_certificate.data[DeviceIdOffset]), sizeof(found_device_id_str) - 1);

            static constexpr u64 DeviceIdLowMask = 0x00FFFFFFFFFFFFFFul;

            return (std::strtoul(found_device_id_str, nullptr, 16) & DeviceIdLowMask) == (device_id & DeviceIdLowMask);
        }

        bool ContainsCorrectDeviceId(const CalibrationInfo &cal) {
            return ContainsCorrectDeviceId(cal.GetBlock<EccB233DeviceCertificateBlock>(), exosphere::GetDeviceId());
        }

        bool IsValidForSecureBackup(const CalibrationInfo &cal) {
            return IsValid(cal) && ContainsCorrectDeviceId(cal);
        }

        bool IsBlank(const CalibrationInfo &cal) {
            return IsBlank(cal.GetBlock<SerialNumberBlock>())                   ||
                   IsBlank(cal.GetBlock<SslCertificateSizeBlock>())             ||
                   IsBlank(cal.GetBlock<SslCertificateBlock>())                 ||
                   IsBlank(cal.GetBlock<EcqvEcdsaAmiiboRootCertificateBlock>()) ||
                   IsBlank(cal.GetBlock<EcqvBlsAmiiboRootCertificateBlock>())   ||
                   IsBlank(cal.GetBlock<ExtendedSslKeyBlock>());
        }

        void ReadStorageCalibrationBinary(CalibrationInfo *out) {
            FsStorage calibration_binary_storage;
            R_ABORT_UNLESS(fsOpenBisStorage(&calibration_binary_storage, FsBisPartitionId_CalibrationBinary));
            ON_SCOPE_EXIT { fsStorageClose(&calibration_binary_storage); };

            R_ABORT_UNLESS(fsStorageRead(&calibration_binary_storage, 0, out, sizeof(*out)));
        }

        constexpr inline const u8 SecureCalibrationBinaryBackupIv[crypto::Aes128CtrDecryptor::IvSize] = {};

        void ReadStorageEncryptedSecureCalibrationBinaryBackupUnsafe(SecureCalibrationInfoBackup *out) {
            FsStorage calibration_binary_storage;
            R_ABORT_UNLESS(fsOpenBisStorage(&calibration_binary_storage, FsBisPartitionId_CalibrationBinary));
            ON_SCOPE_EXIT { fsStorageClose(&calibration_binary_storage); };

            R_ABORT_UNLESS(fsStorageRead(&calibration_binary_storage, SecureCalibrationInfoBackupOffset, out, sizeof(*out)));
        }

        void WriteStorageEncryptedSecureCalibrationBinaryBackupUnsafe(const SecureCalibrationInfoBackup *src) {
            FsStorage calibration_binary_storage;
            R_ABORT_UNLESS(fsOpenBisStorage(&calibration_binary_storage, FsBisPartitionId_CalibrationBinary));
            ON_SCOPE_EXIT { fsStorageClose(&calibration_binary_storage); };

            R_ABORT_UNLESS(fsStorageWrite(&calibration_binary_storage, SecureCalibrationInfoBackupOffset, src, sizeof(*src)));
        }

        void GenerateSecureCalibrationBinaryBackupKey(void *dst, size_t dst_size) {
            static constexpr const u8 SecureCalibrationBinaryBackupKeySource[crypto::Aes128CtrDecryptor::KeySize] = { '|', '-', 'A', 'M', 'S', '-', 'C', 'A', 'L', '0', '-', 'K', 'E', 'Y', '-', '|' };
            spl::AccessKey access_key;
            ON_SCOPE_EXIT { crypto::ClearMemory(std::addressof(access_key), sizeof(access_key)); };

            /* Generate a personalized kek. */
            R_ABORT_UNLESS(spl::GenerateAesKek(std::addressof(access_key), SecureCalibrationBinaryBackupKeySource, sizeof(SecureCalibrationBinaryBackupKeySource), 0, 1));

            /* Generate a personalized key. */
            R_ABORT_UNLESS(spl::GenerateAesKey(dst, dst_size, access_key, SecureCalibrationBinaryBackupKeySource, sizeof(SecureCalibrationBinaryBackupKeySource)));
        }

        bool ReadStorageSecureCalibrationBinaryBackup(SecureCalibrationInfoBackup *out) {
            /* Read the data. */
            ReadStorageEncryptedSecureCalibrationBinaryBackupUnsafe(out);

            /* Don't leak any data unless we validate. */
            auto clear_guard = SCOPE_GUARD { std::memset(out, 0, sizeof(*out)); };

            {
                /* Create a buffer to hold our key. */
                u8 key[crypto::Aes128CtrDecryptor::KeySize];
                ON_SCOPE_EXIT { crypto::ClearMemory(key, sizeof(key)); };

                /* Generate the key. */
                GenerateSecureCalibrationBinaryBackupKey(key, sizeof(key));

                /* Decrypt the data in place. */
                crypto::DecryptAes128Ctr(out, sizeof(*out), key, sizeof(key), SecureCalibrationBinaryBackupIv, sizeof(SecureCalibrationBinaryBackupIv), out, sizeof(*out));
            }

            /* Generate a hash for the data. */
            if (!IsValidSha256Hash(out->hash, std::addressof(out->info), sizeof(out->info))) {
                return false;
            }

            /* Validate the backup. */
            if (!IsValidForSecureBackup(out->info)) {
                return false;
            }

            /* Our backup is valid. */
            clear_guard.Cancel();
            return true;
        }

        void WriteStorageSecureCalibrationBinaryBackup(SecureCalibrationInfoBackup *src) {
            /* Clear the input once we've written it. */
            ON_SCOPE_EXIT { std::memset(src, 0, sizeof(*src)); };

            /* Ensure that the input is valid. */
            AMS_ABORT_UNLESS(IsValidForSecureBackup(src->info));

            /* Set the Sha256 hash. */
            crypto::GenerateSha256Hash(std::addressof(src->hash), sizeof(src->hash), std::addressof(src->info), sizeof(src->info));

            /* Validate the hash. */
            AMS_ABORT_UNLESS(IsValidSha256Hash(src->hash, std::addressof(src->info), sizeof(src->info)));

            /* Encrypt the data. */
            {
                /* Create a buffer to hold our key. */
                u8 key[crypto::Aes128CtrDecryptor::KeySize];
                ON_SCOPE_EXIT { crypto::ClearMemory(key, sizeof(key)); };

                /* Generate the key. */
                GenerateSecureCalibrationBinaryBackupKey(key, sizeof(key));

                /* Encrypt the data in place. */
                crypto::EncryptAes128Ctr(src, sizeof(*src), key, sizeof(key), SecureCalibrationBinaryBackupIv, sizeof(SecureCalibrationBinaryBackupIv), src, sizeof(*src));
            }

            /* Write the encrypted data. */
            WriteStorageEncryptedSecureCalibrationBinaryBackupUnsafe(src);
        }

        void GetBackupFileName(char *dst, size_t dst_size, const CalibrationInfo &info) {
            char sn[0x20] = {};
            ON_SCOPE_EXIT { std::memset(sn, 0, sizeof(sn)); };


            if (IsValidForSecureBackup(info)) {
                GetSerialNumber(sn, info);
                std::snprintf(dst, dst_size, "automatic_backups/%s_PRODINFO.bin", sn);
            } else {
                Sha256Hash hash;
                crypto::GenerateSha256Hash(std::addressof(hash), sizeof(hash), std::addressof(info), sizeof(info));
                ON_SCOPE_EXIT { crypto::ClearMemory(std::addressof(hash), sizeof(hash)); };

                if (IsValid(info)) {
                    if (IsBlank(info)) {
                        std::snprintf(dst, dst_size, "automatic_backups/BLANK_PRODINFO_%02X%02X%02X%02X.bin", hash.data[0], hash.data[1], hash.data[2], hash.data[3]);
                    } else {
                        GetSerialNumber(sn, info);
                        std::snprintf(dst, dst_size, "automatic_backups/%s_PRODINFO_%02X%02X%02X%02X.bin", sn, hash.data[0], hash.data[1], hash.data[2], hash.data[3]);
                    }
                } else {
                    std::snprintf(dst, dst_size, "automatic_backups/INVALID_PRODINFO_%02X%02X%02X%02X.bin", hash.data[0], hash.data[1], hash.data[2], hash.data[3]);
                }
            }
        }

        void SafeRead(ams::fs::fsa::IFile *file, s64 offset, void *dst, size_t size) {
            size_t read_size = 0;
            R_ABORT_UNLESS(file->Read(std::addressof(read_size), offset, dst, size));
            AMS_ABORT_UNLESS(read_size == size);
        }

        alignas(os::MemoryPageSize) CalibrationInfo g_temp_calibration_info = {};

        void SaveProdInfoBackup(std::optional<ams::fs::FileStorage> *dst, const CalibrationInfo &info) {
            char backup_fn[0x100];
            GetBackupFileName(backup_fn, sizeof(backup_fn), info);

            /* Create the file, in case it does not exist. */
            mitm::fs::CreateAtmosphereSdFile(backup_fn, sizeof(CalibrationInfo), ams::fs::CreateOption_None);

            /* Open the file. */
            FsFile libnx_file;
            R_ABORT_UNLESS(mitm::fs::OpenAtmosphereSdFile(std::addressof(libnx_file), backup_fn, ams::fs::OpenMode_ReadWrite));

            /* Create our accessor. */
            std::unique_ptr<ams::fs::fsa::IFile> file = std::make_unique<ams::fs::RemoteFile>(libnx_file);
            AMS_ABORT_UNLESS(file != nullptr);

            /* Check if we're valid already. */
            bool valid = false;
            s64 size;
            R_ABORT_UNLESS(file->GetSize(std::addressof(size)));
            if (size == sizeof(CalibrationInfo)) {
                SafeRead(file.get(), 0, std::addressof(g_temp_calibration_info), sizeof(g_temp_calibration_info));
                ON_SCOPE_EXIT { std::memset(std::addressof(g_temp_calibration_info), 0, sizeof(g_temp_calibration_info)); };

                if (std::memcmp(std::addressof(info), std::addressof(g_temp_calibration_info), sizeof(CalibrationInfo)) == 0) {
                    valid = true;
                }
            }

            /* If we're not valid, we need to save. */
            if (!valid) {
                R_ABORT_UNLESS(file->Write(0, std::addressof(info), sizeof(info), ams::fs::WriteOption::Flush));
            }

            /* Save our storage to output. */
            if (dst != nullptr) {
                dst->emplace(std::move(file));
            }
        }

        void GetRandomEntropy(Sha256Hash *dst) {
            AMS_ASSERT(dst != nullptr);

            u64 data_buffer[3] = {};
            ON_SCOPE_EXIT { crypto::ClearMemory(data_buffer, sizeof(data_buffer)); };

            data_buffer[0] = os::GetSystemTick().GetInt64Value();
            R_ABORT_UNLESS(svc::GetInfo(data_buffer + 1, svc::InfoType_AliasRegionAddress, svc::PseudoHandle::CurrentProcess, 0));
            if (hos::GetVersion() >= hos::Version_2_0_0) {
                R_ABORT_UNLESS(svc::GetInfo(data_buffer + 2, svc::InfoType_RandomEntropy, svc::InvalidHandle, (data_buffer[0] ^ (data_buffer[1] >> 24)) & 3));
            } else {
                data_buffer[2] = os::GetSystemTick().GetInt64Value();
            }

            return crypto::GenerateSha256Hash(dst, sizeof(*dst), data_buffer, sizeof(data_buffer));
        }

        void FillWithGarbage(void *dst, size_t dst_size) {
            /* Get random entropy. */
            Sha256Hash entropy;
            ON_SCOPE_EXIT { crypto::ClearMemory(std::addressof(entropy), sizeof(entropy)); };
            GetRandomEntropy(std::addressof(entropy));

            /* Clear dst. */
            std::memset(dst, 0xCC, dst_size);

            /* Encrypt dst. */
            static_assert(sizeof(entropy) == crypto::Aes128CtrEncryptor::KeySize + crypto::Aes128CtrEncryptor::IvSize);
            crypto::EncryptAes128Ctr(dst, dst_size, entropy.data, crypto::Aes128CtrEncryptor::KeySize, entropy.data + crypto::Aes128CtrEncryptor::KeySize, crypto::Aes128CtrEncryptor::IvSize, dst, dst_size);
        }

        alignas(os::MemoryPageSize) CalibrationInfo g_calibration_info = {};
        alignas(os::MemoryPageSize) CalibrationInfo g_blank_calibration_info = {};
        alignas(os::MemoryPageSize) SecureCalibrationInfoBackup g_secure_calibration_info_backup = {};

        std::optional<ams::fs::FileStorage> g_prodinfo_backup_file;
        std::optional<ams::fs::MemoryStorage> g_blank_prodinfo_storage;
        std::optional<ams::fs::MemoryStorage> g_fake_secure_backup_storage;

        bool g_allow_writes     = false;
        bool g_has_secure_backup = false;

        os::Mutex g_prodinfo_management_lock(false);

    }

    void InitializeProdInfoManagement() {
        std::scoped_lock lk(g_prodinfo_management_lock);

        /* First, get our options. */
        const bool should_blank = exosphere::ShouldBlankProdInfo();
        bool allow_writes = exosphere::ShouldAllowWritesToProdInfo();

        /* Next, read our prodinfo. */
        ReadStorageCalibrationBinary(std::addressof(g_calibration_info));

        /* Next, check if we have a secure backup. */
        bool has_secure_backup = ReadStorageSecureCalibrationBinaryBackup(std::addressof(g_secure_calibration_info_backup));

        /* Only allow writes if we have a secure backup. */
        if (allow_writes && !has_secure_backup) {
            /* If we can make a secure backup, great. */
            if (IsValidForSecureBackup(g_calibration_info)) {
                g_secure_calibration_info_backup.info = g_calibration_info;
                WriteStorageSecureCalibrationBinaryBackup(std::addressof(g_secure_calibration_info_backup));
                g_secure_calibration_info_backup.info = g_calibration_info;
                has_secure_backup = true;
            } else {
                /* Don't allow writes if we can't make a secure backup. */
                allow_writes = false;
            }
        }

        /* Ensure our preconditions are met. */
        AMS_ABORT_UNLESS(!allow_writes || has_secure_backup);

        /* Set globals. */
        g_allow_writes      = allow_writes;
        g_has_secure_backup = has_secure_backup;

        /* If we should blank, do so. */
        if (should_blank) {
            g_blank_calibration_info = g_calibration_info;
            Blank(g_blank_calibration_info);
            g_blank_prodinfo_storage.emplace(std::addressof(g_blank_calibration_info), sizeof(g_blank_calibration_info));
        }

        /* Ensure that we have a blank file only if we need one. */
        AMS_ABORT_UNLESS(should_blank == static_cast<bool>(g_blank_prodinfo_storage));
    }

    void SaveProdInfoBackupsAndWipeMemory(char *out_name, size_t out_name_size) {
        std::scoped_lock lk(g_prodinfo_management_lock);

        ON_SCOPE_EXIT {
            FillWithGarbage(std::addressof(g_calibration_info), sizeof(g_calibration_info));
            FillWithGarbage(std::addressof(g_secure_calibration_info_backup), sizeof(g_secure_calibration_info_backup));
        };

        /* Save our backup. We always prefer to save a secure copy of data over a non-secure one. */
        if (g_has_secure_backup) {
            GetSerialNumber(out_name, g_secure_calibration_info_backup.info);
            SaveProdInfoBackup(std::addressof(g_prodinfo_backup_file), g_secure_calibration_info_backup.info);
        } else {
            if (IsValid(g_calibration_info) && !IsBlank(g_calibration_info)) {
                GetSerialNumber(out_name, g_calibration_info);
            } else {
                Sha256Hash hash;
                ON_SCOPE_EXIT { crypto::ClearMemory(std::addressof(hash), sizeof(hash)); };
                crypto::GenerateSha256Hash(std::addressof(hash), sizeof(hash), std::addressof(g_calibration_info), sizeof(g_calibration_info));

                std::snprintf(out_name, out_name_size, "%02X%02X%02X%02X", hash.data[0], hash.data[1], hash.data[2], hash.data[3]);
            }
            SaveProdInfoBackup(std::addressof(g_prodinfo_backup_file), g_calibration_info);
        }

        /* Ensure we made our backup. */
        AMS_ABORT_UNLESS(g_prodinfo_backup_file);

        /* Setup our memory storage. */
        g_fake_secure_backup_storage.emplace(std::addressof(g_secure_calibration_info_backup), sizeof(g_secure_calibration_info_backup));

        /* Ensure that we have a fake storage. */
        AMS_ABORT_UNLESS(static_cast<bool>(g_fake_secure_backup_storage));
    }

    bool ShouldReadBlankCalibrationBinary() {
        std::scoped_lock lk(g_prodinfo_management_lock);
        return static_cast<bool>(g_blank_prodinfo_storage);
    }

    bool IsWriteToCalibrationBinaryAllowed() {
        std::scoped_lock lk(g_prodinfo_management_lock);
        return g_allow_writes;
    }

    void ReadFromBlankCalibrationBinary(s64 offset, void *dst, size_t size) {
        AMS_ABORT_UNLESS(ShouldReadBlankCalibrationBinary());

        std::scoped_lock lk(g_prodinfo_management_lock);
        R_ABORT_UNLESS(g_blank_prodinfo_storage->Read(offset, dst, size));
    }

    void WriteToBlankCalibrationBinary(s64 offset, const void *src, size_t size) {
        AMS_ABORT_UNLESS(ShouldReadBlankCalibrationBinary());

        std::scoped_lock lk(g_prodinfo_management_lock);
        R_ABORT_UNLESS(g_blank_prodinfo_storage->Write(offset, src, size));
    }

    void ReadFromFakeSecureBackupStorage(s64 offset, void *dst, size_t size) {
        AMS_ABORT_UNLESS(IsWriteToCalibrationBinaryAllowed());

        std::scoped_lock lk(g_prodinfo_management_lock);
        R_ABORT_UNLESS(g_fake_secure_backup_storage->Read(offset, dst, size));
    }

    void WriteToFakeSecureBackupStorage(s64 offset, const void *src, size_t size) {
        AMS_ABORT_UNLESS(IsWriteToCalibrationBinaryAllowed());

        std::scoped_lock lk(g_prodinfo_management_lock);
        R_ABORT_UNLESS(g_fake_secure_backup_storage->Write(offset, src, size));
    }

}
