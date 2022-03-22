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
#include <exosphere.hpp>
#include "fusee_display.hpp"

namespace ams::nxboot {

    constexpr inline const size_t ExternalPackageSize         = 8_MB;

    constexpr inline const size_t InitialProcessStorageSizeMax = 3_MB / 8;

    struct ExternalPackageContentMeta {
        u32 offset;
        u32 size;
        u8 type;
        u8 flags[3];
        u32 pad;
        char name[0x10];
    };
    static_assert(sizeof(ExternalPackageContentMeta) == 0x20);

    struct ExternalPackageKipMeta {
        u64 program_id;
        u32 offset;
        u32 size;
        se::Sha256Hash hash;
    };
    static_assert(sizeof(ExternalPackageKipMeta) == 0x30);

    struct ExternalPackageHeader {
        static constexpr u32 Magic = util::FourCC<'P', 'K', '3', '1'>::Code;
        static constexpr u32 LegacyMagic = util::FourCC<'F','S','S','0'>::Code;

        u32 magic; /* Previously entrypoint. */
        u32 metadata_offset;
        u32 flags;
        u32 meso_size;
        u32 num_kips;
        u32 reserved1[3];
        u32 legacy_magic;
        u32 total_size;
        u32 reserved2; /* Previously crt0 offset. */
        u32 content_header_offset;
        u32 num_content_headers;
        u32 supported_hos_version;
        u32 release_version;
        u32 git_revision;
        ExternalPackageContentMeta content_metas[(0x400 - 0x40) / sizeof(ExternalPackageContentMeta)];
        ExternalPackageKipMeta emummc_meta;
        ExternalPackageKipMeta kip_metas[8];
        u8 reserved3[0x800 - (0x400 + 9 * sizeof(ExternalPackageKipMeta))];
    };
    static_assert(sizeof(ExternalPackageHeader) == 0x800);

    struct ExternalPackage {
        ExternalPackageHeader header;        /* 0x000000-0x000800 */
        u8 warmboot[0x1800];                  /* 0x000800-0x002000 */
        u8 tsec_keygen[0x2000];               /* 0x002000-0x004000 */
        u8 mariko_fatal[0x1C000];             /* 0x004000-0x020000 */
        u8 ovl_mtc_erista[0x14000];           /* 0x020000-0x034000 */
        u8 ovl_mtc_mariko[0x14000];           /* 0x034000-0x048000 */
        u8 exosphere[0xE000];                 /* 0x048000-0x056000 */
        u8 mesosphere[0xAA000];               /* 0x056000-0x100000 */
        u8 kips[3_MB];                        /* 0x100000-0x400000 */
        u8 splash_screen_fb[FrameBufferSize]; /* 0x400000-0x7C0000 */
        u8 fusee[0x20000];                    /* 0x7C0000-0x7E0000 */
        u8 reboot_stub[0x1000];               /* 0x7E0000-0x7E1000 */
        u8 reserved[0x1F000];                 /* 0x7E1000-0x800000 */
    };
    static_assert(sizeof(ExternalPackage) == ExternalPackageSize);

    ALWAYS_INLINE const ExternalPackage &GetExternalPackage() { return *reinterpret_cast<const ExternalPackage *>(0xC0000000); }

}
