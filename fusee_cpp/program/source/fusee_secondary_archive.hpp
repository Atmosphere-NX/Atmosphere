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
#include <exosphere.hpp>
#include "fusee_display.hpp"

namespace ams::nxboot {

    constexpr inline const size_t SecondaryArchiveSize         = 4_MB + FrameBufferSize;

    constexpr inline const size_t InitialProcessStorageSizeMax = 3_MB / 8;

    struct SecondaryArchiveContentMeta {
        u32 offset;
        u32 size;
        u8 type;
        u8 flags[3];
        u32 pad;
        char name[0x10];
    };
    static_assert(sizeof(SecondaryArchiveContentMeta) == 0x20);

    struct SecondaryArchiveKipMeta {
        u64 program_id;
        u32 offset;
        u32 size;
        se::Sha256Hash hash;
    };
    static_assert(sizeof(SecondaryArchiveKipMeta) == 0x30);

    struct SecondaryArchiveHeader {
        static constexpr u32 Magic = util::FourCC<'F','S','S','0'>::Code;

        u32 reserved0; /* Previously entrypoint. */
        u32 metadata_offset;
        u32 reserved1;
        u32 num_kips;
        u32 reserved2[4];
        u32 magic;
        u32 total_size;
        u32 reserved3; /* Previously crt0 offset. */
        u32 content_header_offset;
        u32 num_content_headers;
        u32 supported_hos_version;
        u32 release_version;
        u32 git_revision;
        SecondaryArchiveContentMeta content_metas[(0x400 - 0x40) / sizeof(SecondaryArchiveContentMeta)];
        SecondaryArchiveKipMeta emummc_meta;
        SecondaryArchiveKipMeta kip_metas[8];
        u8 reserved4[0x800 - (0x400 + 9 * sizeof(SecondaryArchiveKipMeta))];
    };
    static_assert(sizeof(SecondaryArchiveHeader) == 0x800);

    struct SecondaryArchive {
        SecondaryArchiveHeader header;        /* 0x000000-0x000800 */
        u8 warmboot[0x1800];                  /* 0x000800-0x002000 */
        u8 tsec_keygen[0x2000];               /* 0x002000-0x004000 */
        u8 mariko_fatal[0x1C000];             /* 0x004000-0x020000 */
        u8 ovl_mtc_erista[0x14000];           /* 0x020000-0x034000 */
        u8 ovl_mtc_mariko[0x14000];           /* 0x034000-0x048000 */
        u8 exosphere[0xE000];                 /* 0x048000-0x056000 */
        u8 mesosphere[0xAA000];               /* 0x056000-0x100000 */
        u8 kips[3_MB];                        /* 0x100000-0x400000 */
        u8 splash_screen_fb[FrameBufferSize]; /* 0x400000-0x7C0000 */
    };
    static_assert(sizeof(SecondaryArchive) == SecondaryArchiveSize);

    ALWAYS_INLINE const SecondaryArchive &GetSecondaryArchive() { return *reinterpret_cast<const SecondaryArchive *>(0xC0000000); }

}
