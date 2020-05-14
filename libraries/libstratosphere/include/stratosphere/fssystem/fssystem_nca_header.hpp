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
#pragma once
#include <vapours.hpp>

namespace ams::fssystem {

    struct Hash {
        static constexpr size_t Size = crypto::Sha256Generator::HashSize;
        u8 value[Size];
    };
    static_assert(sizeof(Hash) == Hash::Size);
    static_assert(util::is_pod<Hash>::value);

    using NcaDigest = Hash;

    struct NcaHeader {
        enum class ContentType : u8 {
            Program    = 0,
            Meta       = 1,
            Control    = 2,
            Manual     = 3,
            Data       = 4,
            PublicData = 5,

            Start = Program,
            End   = PublicData,
        };

        enum class DistributionType : u8 {
            Download = 0,
            GameCard = 1,

            Start = Download,
            End   = GameCard,
        };

        enum class EncryptionType : u8 {
            Auto = 0,
            None = 1,
        };

        enum DecryptionKey {
            DecryptionKey_AesXts   = 0,
            DecryptionKey_AesXts1  = DecryptionKey_AesXts,
            DecryptionKey_AesXts2  = 1,
            DecryptionKey_AesCtr   = 2,
            DecryptionKey_AesCtrEx = 3,
            DecryptionKey_AesCtrHw = 4,
            DecryptionKey_Count,
        };

        struct FsInfo {
            u32 start_sector;
            u32 end_sector;
            u32 hash_sectors;
            u32 reserved;
        };
        static_assert(sizeof(FsInfo) == 0x10);
        static_assert(util::is_pod<FsInfo>::value);

        static constexpr u32 Magic0 = util::FourCC<'N','C','A','0'>::Code;
        static constexpr u32 Magic1 = util::FourCC<'N','C','A','1'>::Code;
        static constexpr u32 Magic2 = util::FourCC<'N','C','A','2'>::Code;
        static constexpr u32 Magic3 = util::FourCC<'N','C','A','3'>::Code;

        static constexpr u32 Magic              = Magic3;

        static constexpr size_t Size                             = 1_KB;
        static constexpr s32    FsCountMax                       = 4;
        static constexpr size_t HeaderSignCount                  = 2;
        static constexpr size_t HeaderSignSize                   = 0x100;
        static constexpr size_t EncryptedKeyAreaSize             = 0x100;
        static constexpr size_t SectorSize                       = 0x200;
        static constexpr size_t SectorShift                      = 9;
        static constexpr size_t RightsIdSize                     = 0x10;
        static constexpr size_t XtsBlockSize                     = 0x200;
        static constexpr size_t CtrBlockSize                     = 0x10;

        static_assert(SectorSize == (1 << SectorShift));

        /* Data members. */
        u8 header_sign_1[HeaderSignSize];
        u8 header_sign_2[HeaderSignSize];
        u32 magic;
        DistributionType distribution_type;
        ContentType content_type;
        u8 key_generation;
        u8 key_index;
        u64 content_size;
        u64 program_id;
        u32 content_index;
        u32 sdk_addon_version;
        u8 key_generation_2;
        u8 header1_signature_key_generation;
        u8 reserved_222[2];
        u32 reserved_224[3];
        u8 rights_id[RightsIdSize];
        FsInfo fs_info[FsCountMax];
        Hash fs_header_hash[FsCountMax];
        u8 encrypted_key_area[EncryptedKeyAreaSize];

        static constexpr u64 SectorToByte(u32 sector) {
            return static_cast<u64>(sector) << SectorShift;
        }

        static constexpr u32 ByteToSector(u64 byte) {
            return static_cast<u32>(byte >> SectorShift);
        }

        u8 GetProperKeyGeneration() const;
    };
    static_assert(sizeof(NcaHeader) == NcaHeader::Size);
    static_assert(util::is_pod<NcaHeader>::value);

    struct NcaBucketInfo {
        static constexpr size_t HeaderSize = 0x10;
        s64 offset;
        s64 size;
        u8 header[HeaderSize];
    };
    static_assert(util::is_pod<NcaBucketInfo>::value);

    struct NcaPatchInfo {
        static constexpr size_t Size   = 0x40;
        static constexpr size_t Offset = 0x100;

        s64 indirect_offset;
        s64 indirect_size;
        u8  indirect_header[NcaBucketInfo::HeaderSize];
        s64 aes_ctr_ex_offset;
        s64 aes_ctr_ex_size;
        u8  aes_ctr_ex_header[NcaBucketInfo::HeaderSize];

        bool HasIndirectTable() const;
        bool HasAesCtrExTable() const;
    };
    static_assert(util::is_pod<NcaPatchInfo>::value);

    union NcaAesCtrUpperIv {
        u64 value;
        struct {
            u32 generation;
            u32 secure_value;
        } part;
    };
    static_assert(util::is_pod<NcaAesCtrUpperIv>::value);

    struct NcaSparseInfo {
        NcaBucketInfo bucket;
        s64 physical_offset;
        u16 generation;
        u8  reserved[6];

        s64 GetPhysicalSize() const {
            return this->bucket.offset + this->bucket.size;
        }

        u32 GetGeneration() const {
            return static_cast<u32>(this->generation) << 16;
        }

        const NcaAesCtrUpperIv MakeAesCtrUpperIv(NcaAesCtrUpperIv upper_iv) const {
            NcaAesCtrUpperIv sparse_upper_iv = upper_iv;
            sparse_upper_iv.part.generation = this->GetGeneration();
            return sparse_upper_iv;
        }
    };
    static_assert(util::is_pod<NcaSparseInfo>::value);

    struct NcaFsHeader {
        static constexpr size_t Size           = 0x200;
        static constexpr size_t HashDataOffset = 0x8;

        struct Region {
            s64 offset;
            s64 size;
        };
        static_assert(util::is_pod<Region>::value);

        enum class FsType : u8 {
            RomFs       = 0,
            PartitionFs = 1,
        };

        enum class EncryptionType : u8 {
            Auto     = 0,
            None     = 1,
            AesXts   = 2,
            AesCtr   = 3,
            AesCtrEx = 4,
        };

        enum class HashType : u8 {
            Auto                      = 0,
            None                      = 1,
            HierarchicalSha256Hash    = 2,
            HierarchicalIntegrityHash = 3,
        };

        union HashData {
            struct HierarchicalSha256Data {
                static constexpr size_t HashLayerCountMax = 5;
                static const size_t MasterHashOffset;

                Hash fs_data_master_hash;
                s32 hash_block_size;
                s32 hash_layer_count;
                Region hash_layer_region[HashLayerCountMax];
            } hierarchical_sha256_data;
            static_assert(util::is_pod<HierarchicalSha256Data>::value);

            struct IntegrityMetaInfo {
                static const size_t MasterHashOffset;

                u32 magic;
                u32 version;
                u32 master_hash_size;

                struct LevelHashInfo {
                    u32 max_layers;

                    struct HierarchicalIntegrityVerificationLevelInformation {
                        static constexpr size_t IntegrityMaxLayerCount = 7;
                        s64 offset;
                        s64 size;
                        s32 block_order;
                        u8  reserved[4];
                    } info[HierarchicalIntegrityVerificationLevelInformation::IntegrityMaxLayerCount - 1];

                    struct SignatureSalt {
                        static constexpr size_t Size = 0x20;
                        u8 value[Size];
                    } seed;
                } level_hash_info;

                Hash master_hash;
            } integrity_meta_info;
            static_assert(util::is_pod<IntegrityMetaInfo>::value);

            u8 padding[NcaPatchInfo::Offset - HashDataOffset];
        };

        u16 version;
        FsType fs_type;
        HashType hash_type;
        EncryptionType encryption_type;
        u8 reserved[3];
        HashData hash_data;
        NcaPatchInfo patch_info;
        NcaAesCtrUpperIv aes_ctr_upper_iv;
        NcaSparseInfo sparse_info;
        u8 pad[0x88];
    };
    static_assert(sizeof(NcaFsHeader) == NcaFsHeader::Size);
    static_assert(util::is_pod<NcaFsHeader>::value);
    static_assert(offsetof(NcaFsHeader, patch_info) == NcaPatchInfo::Offset);

    inline constexpr const size_t NcaFsHeader::HashData::HierarchicalSha256Data::MasterHashOffset = offsetof(NcaFsHeader, hash_data.hierarchical_sha256_data.fs_data_master_hash);
    inline constexpr const size_t NcaFsHeader::HashData::IntegrityMetaInfo::MasterHashOffset      = offsetof(NcaFsHeader, hash_data.integrity_meta_info.master_hash);

}
