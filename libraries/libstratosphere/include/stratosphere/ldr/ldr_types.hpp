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
#include <stratosphere/ncm/ncm_ids.hpp>
#include <stratosphere/ncm/ncm_program_location.hpp>
#include <stratosphere/sf/sf_buffer_tags.hpp>

namespace ams::ldr {

    /* General types. */
    struct ProgramInfo : sf::LargeData {
        u8 main_thread_priority;
        u8 default_cpu_id;
        u16 flags;
        u32 main_thread_stack_size;
        ncm::ProgramId program_id;
        u32 acid_sac_size;
        u32 aci_sac_size;
        u32 acid_fac_size;
        u32 aci_fah_size;
        u8 ac_buffer[0x3E0];
    };
    static_assert(std::is_pod<ProgramInfo>::value && sizeof(ProgramInfo) == 0x400, "ProgramInfo definition!");

    enum ProgramInfoFlag {
        ProgramInfoFlag_SystemModule        = (0 << 0),
        ProgramInfoFlag_Application         = (1 << 0),
        ProgramInfoFlag_Applet              = (2 << 0),
        ProgramInfoFlag_InvalidType         = (3 << 0),
        ProgramInfoFlag_ApplicationTypeMask = (3 << 0),

        ProgramInfoFlag_AllowDebug = (1 << 2),
    };

    enum CreateProcessFlag {
        CreateProcessFlag_EnableDebug = (1 << 0),
        CreateProcessFlag_DisableAslr = (1 << 1),
    };

    struct ProgramArguments {
        u32 allocated_size;
        u32 arguments_size;
        u8  reserved[0x18];
        u8  arguments[];
    };
    static_assert(sizeof(ProgramArguments) == 0x20, "ProgramArguments definition!");

    struct PinId {
        u64 value;
    };

    inline bool operator==(const PinId &lhs, const PinId &rhs) {
        return lhs.value == rhs.value;
    }

    inline bool operator!=(const PinId &lhs, const PinId &rhs) {
        return lhs.value != rhs.value;
    }
    static_assert(sizeof(PinId) == sizeof(u64) && std::is_pod<PinId>::value, "PinId definition!");

    /* Import ModuleInfo from libnx. */
    using ModuleInfo = ::LoaderModuleInfo;

    /* NSO types. */
    struct NsoHeader {
        static constexpr u32 Magic = util::FourCC<'N','S','O','0'>::Code;
        enum Segment : size_t {
            Segment_Text = 0,
            Segment_Ro   = 1,
            Segment_Rw   = 2,
            Segment_Count,
        };

        enum Flag : u32 {
            Flag_CompressedText = (1 << 0),
            Flag_CompressedRo   = (1 << 1),
            Flag_CompressedRw   = (1 << 2),
            Flag_CheckHashText     = (1 << 3),
            Flag_CheckHashRo       = (1 << 4),
            Flag_CheckHashRw       = (1 << 5),
        };

        struct SegmentInfo {
            u32 file_offset;
            u32 dst_offset;
            u32 size;
            u32 reserved;
        };

        u32 magic;
        u32 version;
        u32 reserved_08;
        u32 flags;
        union {
            struct {
                u32 text_file_offset;
                u32 text_dst_offset;
                u32 text_size;
                u32 unk_file_offset;
                u32 ro_file_offset;
                u32 ro_dst_offset;
                u32 ro_size;
                u32 unk_size;
                u32 rw_file_offset;
                u32 rw_dst_offset;
                u32 rw_size;
                u32 bss_size;
            };
            SegmentInfo segments[Segment_Count];
        };
        u8 build_id[sizeof(ModuleInfo::build_id)];
        union {
            u32 compressed_sizes[Segment_Count];
            struct {
                u32 text_compressed_size;
                u32 ro_compressed_size;
                u32 rw_compressed_size;
            };
        };
        u8 reserved_6C[0x34];
        union {
            u8 segment_hashes[Segment_Count][SHA256_HASH_SIZE];
            struct {
                u8 text_hash[SHA256_HASH_SIZE];
                u8 ro_hash[SHA256_HASH_SIZE];
                u8 rw_hash[SHA256_HASH_SIZE];
            };
        };
    };
    static_assert(sizeof(NsoHeader) == 0x100 && std::is_pod<NsoHeader>::value, "NsoHeader definition!");

    /* NPDM types. */
    struct Aci {
        static constexpr u32 Magic = util::FourCC<'A','C','I','0'>::Code;

        u32 magic;
        u8  reserved_04[0xC];
        ncm::ProgramId program_id;
        u8  reserved_18[0x8];
        u32 fah_offset;
        u32 fah_size;
        u32 sac_offset;
        u32 sac_size;
        u32 kac_offset;
        u32 kac_size;
        u8  reserved_38[0x8];
    };
    static_assert(sizeof(Aci) == 0x40 && std::is_pod<Aci>::value, "Aci definition!");

    struct Acid {
        static constexpr u32 Magic = util::FourCC<'A','C','I','D'>::Code;

        enum AcidFlag {
            AcidFlag_Production             = (1 << 0),
            AcidFlag_UnqualifiedApproval    = (1 << 1),

            AcidFlag_DeprecatedUseSecureMemory = (1 << 2),

            AcidFlag_PoolPartitionShift = 2,
            AcidFlag_PoolPartitionMask = (3 << AcidFlag_PoolPartitionShift),
        };

        enum PoolPartition {
            PoolPartition_Application     = (svc::CreateProcessFlag_PoolPartitionApplication     >> svc::CreateProcessFlag_PoolPartitionShift),
            PoolPartition_Applet          = (svc::CreateProcessFlag_PoolPartitionApplet          >> svc::CreateProcessFlag_PoolPartitionShift),
            PoolPartition_System          = (svc::CreateProcessFlag_PoolPartitionSystem          >> svc::CreateProcessFlag_PoolPartitionShift),
            PoolPartition_SystemNonSecure = (svc::CreateProcessFlag_PoolPartitionSystemNonSecure >> svc::CreateProcessFlag_PoolPartitionShift),
        };

        u8 signature[0x100];
        u8 modulus[0x100];
        u32 magic;
        u32 size;
        u8  version;
        u8  reserved_209[3];
        u32 flags;
        ncm::ProgramId program_id_min;
        ncm::ProgramId program_id_max;
        u32 fac_offset;
        u32 fac_size;
        u32 sac_offset;
        u32 sac_size;
        u32 kac_offset;
        u32 kac_size;
        u8  reserved_238[0x8];
    };
    static_assert(sizeof(Acid) == 0x240 && std::is_pod<Acid>::value, "Acid definition!");

    struct Npdm {
        static constexpr u32 Magic = util::FourCC<'M','E','T','A'>::Code;

        enum MetaFlag {
            MetaFlag_Is64Bit = (1 << 0),

            MetaFlag_AddressSpaceTypeShift = 1,
            MetaFlag_AddressSpaceTypeMask = (7 << MetaFlag_AddressSpaceTypeShift),

            MetaFlag_OptimizeMemoryAllocation = (1 << 4),
        };

        enum AddressSpaceType {
            AddressSpaceType_32Bit              = (svc::CreateProcessFlag_AddressSpace32Bit             >> svc::CreateProcessFlag_AddressSpaceShift),
            AddressSpaceType_64BitDeprecated    = (svc::CreateProcessFlag_AddressSpace64BitDeprecated   >> svc::CreateProcessFlag_AddressSpaceShift),
            AddressSpaceType_32BitWithoutAlias  = (svc::CreateProcessFlag_AddressSpace32BitWithoutAlias >> svc::CreateProcessFlag_AddressSpaceShift),
            AddressSpaceType_64Bit              = (svc::CreateProcessFlag_AddressSpace64Bit             >> svc::CreateProcessFlag_AddressSpaceShift),
        };

        u32 magic;
        u8  reserved_04[8];
        u8  flags;
        u8  reserved_0D;
        u8 main_thread_priority;
        u8 default_cpu_id;
        u8  reserved_10[4];
        u32 system_resource_size;
        u32 version;
        u32 main_thread_stack_size;
        char program_name[0x10];
        char product_code[0x10];
        u8  reserved_40[0x30];
        u32 aci_offset;
        u32 aci_size;
        u32 acid_offset;
        u32 acid_size;
    };
    static_assert(sizeof(Npdm) == 0x80 && std::is_pod<Npdm>::value, "Npdm definition!");

}
