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

namespace ams::ro {

    enum NrrKind : u8 {
        NrrKind_User      = 0,
        NrrKind_JitPlugin = 1,

        NrrKind_Count,
    };

    struct ModuleId {
        u8 build_id[0x20];
    };
    static_assert(sizeof(ModuleId) == sizeof(LoaderModuleInfo::build_id), "ModuleId definition!");

    struct NrrCertification {
        static constexpr size_t RsaKeySize = 0x100;
        static constexpr size_t SignedSize = 0x120;

        u64 program_id_mask;
        u64 program_id_pattern;
        u8  reserved_10[0x10];
        u8  modulus[RsaKeySize];
        u8  signature[RsaKeySize];
    };
    static_assert(sizeof(NrrCertification) == NrrCertification::RsaKeySize + NrrCertification::SignedSize);

    class NrrHeader {
        public:
            static constexpr u32 Magic = util::FourCC<'N','R','R','0'>::Code;
        private:
            u32 magic;
            u32 key_generation;
            u8  reserved_08[0x08];
            NrrCertification certification;
            u8  signature[0x100];
            ncm::ProgramId program_id;
            u32 size;
            u8  nrr_kind; /* 7.0.0+ */
            u8  reserved_33D[3];
            u32 hashes_offset;
            u32 num_hashes;
            u8  reserved_348[8];
        public:
            bool IsMagicValid() const {
                return this->magic == Magic;
            }

            bool IsProgramIdValid() const {
                return (static_cast<u64>(this->program_id) & this->certification.program_id_mask) == this->certification.program_id_pattern;
            }

            NrrKind GetNrrKind() const {
                const NrrKind kind = static_cast<NrrKind>(this->nrr_kind);
                AMS_ABORT_UNLESS(kind < NrrKind_Count);
                return kind;
            }

            ncm::ProgramId GetProgramId() const {
                return this->program_id;
            }

            u32 GetSize() const {
                return this->size;
            }

            u32 GetNumHashes() const {
                return this->num_hashes;
            }

            size_t GetHashesOffset() const {
                return this->hashes_offset;
            }

            uintptr_t GetHashes() const {
                return reinterpret_cast<uintptr_t>(this) + this->GetHashesOffset();
            }

            u32 GetKeyGeneration() const {
                return this->key_generation;
            }

            const u8 *GetCertificationSignature() const {
                return this->certification.signature;
            }

            const u8 *GetCertificationSignedArea() const {
                return reinterpret_cast<const u8 *>(std::addressof(this->certification));
            }

            const u8 *GetCertificationModulus() const {
                return this->certification.modulus;
            }

            const u8 *GetSignature() const {
                return this->signature;
            }

            const u8 *GetSignedArea() const {
                return reinterpret_cast<const u8 *>(std::addressof(this->program_id));
            }

            size_t GetSignedAreaSize() const {
                return this->size - GetSignedAreaOffset();
            }

            static constexpr size_t GetSignedAreaOffset();
    };
    static_assert(sizeof(NrrHeader) == 0x350, "NrrHeader definition!");

    constexpr size_t NrrHeader::GetSignedAreaOffset() {
        return OFFSETOF(NrrHeader, program_id);
    }

    class NroHeader {
        public:
            static constexpr u32 Magic = util::FourCC<'N','R','O','0'>::Code;
        private:
            u32 entrypoint_insn;
            u32 mod_offset;
            u8  reserved_08[0x8];
            u32 magic;
            u8  reserved_14[0x4];
            u32 size;
            u8  reserved_1C[0x4];
            u32 text_offset;
            u32 text_size;
            u32 ro_offset;
            u32 ro_size;
            u32 rw_offset;
            u32 rw_size;
            u32 bss_size;
            u8  reserved_3C[0x4];
            ModuleId module_id;
            u8  reserved_60[0x20];
        public:
            bool IsMagicValid() const {
                return this->magic == Magic;
            }

            u32 GetSize() const {
                return this->size;
            }

            u32 GetTextOffset() const {
                return this->text_offset;
            }

            u32 GetTextSize() const {
                return this->text_size;
            }

            u32 GetRoOffset() const {
                return this->ro_offset;
            }

            u32 GetRoSize() const {
                return this->ro_size;
            }

            u32 GetRwOffset() const {
                return this->rw_offset;
            }

            u32 GetRwSize() const {
                return this->rw_size;
            }

            u32 GetBssSize() const {
                return this->bss_size;
            }

            const ModuleId *GetModuleId() const {
                return &this->module_id;
            }
    };
    static_assert(sizeof(NroHeader) == 0x80, "NroHeader definition!");

}
