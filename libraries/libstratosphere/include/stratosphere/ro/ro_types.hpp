/*
 * Copyright (c) Atmosphère-NX
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

    static constexpr size_t ModuleIdSize = 0x20;
    struct ModuleId {
        u8 data[ModuleIdSize];
    };
    static_assert(sizeof(ModuleId) == ModuleIdSize);

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
            u32 m_magic;
            u32 m_key_generation;
            u8  m_reserved_08[0x08];
            NrrCertification m_certification;
            u8  m_signature[0x100];
            ncm::ProgramId m_program_id;
            u32 m_size;
            u8  m_nrr_kind; /* 7.0.0+ */
            u8  m_reserved_33D[3];
            u32 m_hashes_offset;
            u32 m_num_hashes;
            u8  m_reserved_348[8];
        public:
            bool IsMagicValid() const {
                return m_magic == Magic;
            }

            bool IsProgramIdValid() const {
                return (static_cast<u64>(m_program_id) & m_certification.program_id_mask) == m_certification.program_id_pattern;
            }

            NrrKind GetNrrKind() const {
                const NrrKind kind = static_cast<NrrKind>(m_nrr_kind);
                AMS_ABORT_UNLESS(kind < NrrKind_Count);
                return kind;
            }

            ncm::ProgramId GetProgramId() const {
                return m_program_id;
            }

            u32 GetSize() const {
                return m_size;
            }

            u32 GetNumHashes() const {
                return m_num_hashes;
            }

            size_t GetHashesOffset() const {
                return m_hashes_offset;
            }

            uintptr_t GetHashes() const {
                return reinterpret_cast<uintptr_t>(this) + this->GetHashesOffset();
            }

            u32 GetKeyGeneration() const {
                return m_key_generation;
            }

            const u8 *GetCertificationSignature() const {
                return m_certification.signature;
            }

            const u8 *GetCertificationSignedArea() const {
                return reinterpret_cast<const u8 *>(std::addressof(m_certification));
            }

            const u8 *GetCertificationModulus() const {
                return m_certification.modulus;
            }

            const u8 *GetSignature() const {
                return m_signature;
            }

            const u8 *GetSignedArea() const {
                return reinterpret_cast<const u8 *>(std::addressof(m_program_id));
            }

            size_t GetSignedAreaSize() const {
                return m_size - GetSignedAreaOffset();
            }

            static constexpr size_t GetSignedAreaOffset();
    };
    static_assert(sizeof(NrrHeader) == 0x350, "NrrHeader definition!");

    constexpr size_t NrrHeader::GetSignedAreaOffset() {
        return AMS_OFFSETOF(NrrHeader, m_program_id);
    }

    class NroHeader {
        public:
            static constexpr u32 Magic = util::FourCC<'N','R','O','0'>::Code;
        private:
            u32 m_entrypoint_insn;
            u32 m_mod_offset;
            u8  m_reserved_08[0x8];
            u32 m_magic;
            u8  m_reserved_14[0x4];
            u32 m_size;
            u8  m_reserved_1C[0x4];
            u32 m_text_offset;
            u32 m_text_size;
            u32 m_ro_offset;
            u32 m_ro_size;
            u32 m_rw_offset;
            u32 m_rw_size;
            u32 m_bss_size;
            u8  m_reserved_3C[0x4];
            ModuleId m_module_id;
            u8  m_reserved_60[0x20];
        public:
            bool IsMagicValid() const {
                return m_magic == Magic;
            }

            u32 GetSize() const {
                return m_size;
            }

            u32 GetTextOffset() const {
                return m_text_offset;
            }

            u32 GetTextSize() const {
                return m_text_size;
            }

            u32 GetRoOffset() const {
                return m_ro_offset;
            }

            u32 GetRoSize() const {
                return m_ro_size;
            }

            u32 GetRwOffset() const {
                return m_rw_offset;
            }

            u32 GetRwSize() const {
                return m_rw_size;
            }

            u32 GetBssSize() const {
                return m_bss_size;
            }

            const ModuleId *GetModuleId() const {
                return std::addressof(m_module_id);
            }
    };
    static_assert(sizeof(NroHeader) == 0x80, "NroHeader definition!");

}
