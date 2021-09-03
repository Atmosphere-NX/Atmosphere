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
#include <exosphere.hpp>
#include "fusee_overlay_manager.hpp"
#include "fusee_secondary_archive.hpp"
#include "fusee_fatal.hpp"

namespace ams::nxboot {

    namespace {

        ALWAYS_INLINE void *GetOverlayDestination() {
            return reinterpret_cast<void *>(0x4002C000);
        }

        void LoadMemoryTrainingOverlay(fs::FileHandle archive_file) {
            Result result;
            u32 verif_hash;
            u32 store_hash;
            if (fuse::GetSocType() == fuse::SocType_Erista) {
                result = fs::ReadFile(archive_file, __builtin_offsetof(SecondaryArchive, ovl_mtc_erista), GetOverlayDestination(), sizeof(SecondaryArchive{}.ovl_mtc_erista));
                verif_hash = reinterpret_cast<const u32 *>(GetOverlayDestination())[-2];
                store_hash = reinterpret_cast<const u32 *>(GetOverlayDestination())[(sizeof(SecondaryArchive{}.ovl_mtc_erista) / sizeof(u32)) - 1];
            } else /* if (fuse::GetSocType() == fuse::SocType_Mariko) */ {
                result = fs::ReadFile(archive_file, __builtin_offsetof(SecondaryArchive, ovl_mtc_mariko), GetOverlayDestination(), sizeof(SecondaryArchive{}.ovl_mtc_mariko));
                verif_hash = reinterpret_cast<const u32 *>(GetOverlayDestination())[-1];
                store_hash = reinterpret_cast<const u32 *>(GetOverlayDestination())[(sizeof(SecondaryArchive{}.ovl_mtc_mariko) / sizeof(u32)) - 1];
            }

            if (R_FAILED(result)) {
                ShowFatalError("Failed to load MTC overlay: 0x%08" PRIx32 "\n", result.GetValue());
            }

            if (verif_hash != store_hash) {
                ShowFatalError("Incorrect fusee version! (program=0x%08" PRIx32 ", mtc=0x%08" PRIx32 ")\n", verif_hash, store_hash);
            }
        }

    }

    void LoadOverlay(fs::FileHandle archive_file, OverlayId ovl) {
        switch (ovl) {
            case OverlayId_MemoryTraining:
                LoadMemoryTrainingOverlay(archive_file);
                break;
        }
    }

    void SaveMemoryTrainingOverlay() {
        if (fuse::GetSocType() == fuse::SocType_Erista) {
            /* NOTE: Erista does not do memory clock restoration. */
            /* std::memcpy(const_cast<u8 *>(GetSecondaryArchive().ovl_mtc_erista), GetOverlayDestination(), sizeof(SecondaryArchive{}.ovl_mtc_erista)); */
        } else /* if (fuse::GetSocType() == fuse::SocType_Mariko) */ {
            std::memcpy(const_cast<u8 *>(GetSecondaryArchive().ovl_mtc_mariko), GetOverlayDestination(), sizeof(SecondaryArchive{}.ovl_mtc_mariko));
        }
    }

    void RestoreMemoryTrainingOverlay() {
        if (fuse::GetSocType() == fuse::SocType_Erista) {
            /* NOTE: Erista does not do memory clock restoration. */
            /* std::memcpy(GetOverlayDestination(), GetSecondaryArchive().ovl_mtc_erista, sizeof(SecondaryArchive{}.ovl_mtc_erista)); */
        } else /* if (fuse::GetSocType() == fuse::SocType_Mariko) */ {
            std::memcpy(GetOverlayDestination(), GetSecondaryArchive().ovl_mtc_mariko, sizeof(SecondaryArchive{}.ovl_mtc_mariko));
        }
    }

}
