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

namespace ams::pkg2 {

    constexpr inline size_t Package2SizeMax  = 8_MB - 16_KB;
    constexpr inline size_t PayloadAlignment = 4;

    constexpr inline int PayloadCount = 3;

    constexpr inline int MinimumValidDataVersion  = 0;   /* We allow older package2 to load; this value is currently 0x18 in Nintendo's code. */
    constexpr inline int CurrentBootloaderVersion = 0x17;

    struct Package2Meta {
        using Magic = util::FourCC<'P','K','2','1'>;

        u32 package2_size;
        u8  key_generation;
        u8  header_iv_remainder[11];
        u8  payload_ivs[PayloadCount][0x10];
        u8  padding_40[0x10];
        u8  magic[4];
        u32 entrypoint;
        u8  padding_58[4];
        u8  package2_version;
        u8  bootloader_version;
        u8  padding_5E[2];
        u32 payload_sizes[PayloadCount];
        u8  padding_6C[4];
        u32 payload_offsets[PayloadCount];
        u8  padding_7C[4];
        u8  payload_hashes[PayloadCount][crypto::Sha256Generator::HashSize];
        u8  padding_E0[0x20];

        private:
            static ALWAYS_INLINE u32 ReadWord(const void *ptr, int offset) {
                return util::LoadLittleEndian(reinterpret_cast<const u32 *>(reinterpret_cast<uintptr_t>(ptr) + offset));
            }
        public:
            ALWAYS_INLINE u8 GetKeyGeneration() const {
                return static_cast<u8>(std::max<s32>(0, static_cast<s32>(this->key_generation ^ this->header_iv_remainder[1] ^ this->header_iv_remainder[2]) - 1));
            }

            ALWAYS_INLINE u32 GetSize() const {
                return this->package2_size ^ ReadWord(this->header_iv_remainder, 3) ^ ReadWord(this->header_iv_remainder, 7);
            }
    };
    static_assert(util::is_pod<Package2Meta>::value);
    static_assert(sizeof(Package2Meta) == 0x100);

    struct Package2Header {
        u8 signature[0x100];
        Package2Meta meta;
    };
    static_assert(util::is_pod<Package2Header>::value);
    static_assert(sizeof(Package2Header) == 0x200);

    struct StorageLayout {
        u8 boot_config[16_KB];
        Package2Header package2_header;
        u8 data[Package2SizeMax - sizeof(Package2Header)];
    };
    static_assert(util::is_pod<StorageLayout>::value);
    static_assert(sizeof(StorageLayout) == 8_MB);

}
