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

#pragma once
#include <atmosphere/common.hpp>
#include "../ncm/ncm_types.hpp"

namespace ams::ro {

    enum class ModuleType : u8 {
        ForSelf   = 0,
        ForOthers = 1,
        Count
    };

    struct ModuleId {
        u8 build_id[0x20];
    };
    static_assert(sizeof(ModuleId) == sizeof(LoaderModuleInfo::build_id), "ModuleId definition!");

    class NrrHeader {
        public:
            static constexpr u32 Magic = 0x3052524E;
        private:
            u32 magic;
            u8  reserved_04[0xC];
            u64 program_id_mask;
            u64 program_id_pattern;
            u8  reserved_20[0x10];
            u8  modulus[0x100];
            u8  fixed_key_signature[0x100];
            u8  nrr_signature[0x100];
            ncm::ProgramId program_id;
            u32 size;
            u8  type; /* 7.0.0+ */
            u8  reserved_33D[3];
            u32 hashes_offset;
            u32 num_hashes;
            u8  reserved_348[8];
        public:
            bool IsMagicValid() const {
                return this->magic == Magic;
            }

            bool IsProgramIdValid() const {
                return (static_cast<u64>(this->program_id) & this->program_id_mask) == this->program_id_pattern;
            }

            ModuleType GetType() const {
                const ModuleType type = static_cast<ModuleType>(this->type);
                AMS_ASSERT(type < ModuleType::Count);
                return type;
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

            uintptr_t GetHashes() const {
                return reinterpret_cast<uintptr_t>(this) + this->hashes_offset;
            }
    };
    static_assert(sizeof(NrrHeader) == 0x350, "NrrHeader definition!");

    class NroHeader {
        public:
            static constexpr u32 Magic = 0x304F524E;
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
