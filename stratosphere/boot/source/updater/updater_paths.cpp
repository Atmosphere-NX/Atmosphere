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

#include <switch.h>
#include <stratosphere.hpp>
#include <sys/stat.h>

#include "updater_api.hpp"

static const char *BootImagePackageMountPath = "bip";
static const char *BctPathNx = "bip:/nx/bct";
static const char *Package1PathNx = "bip:/nx/package1";
static const char *Package2PathNx = "bip:/nx/package2";
static const char *BctPathA = "bip:/a/bct";
static const char *Package1PathA = "bip:/a/package1";
static const char *Package2PathA = "bip:/a/package2";

const char *Updater::GetBootImagePackageMountPath() {
    return BootImagePackageMountPath;
}

static const char *ChooseCandidatePath(const char **candidates, size_t num_candidates) {
    if (num_candidates == 0) {
        std::abort();
    }

    for (size_t i = 0; i < num_candidates; i++) {
        struct stat buf;
        if (stat(candidates[i], &buf) != 0) {
            continue;
        }

        if (!S_ISREG(buf.st_mode)) {
            continue;
        }

        return candidates[i];
    }

    /* Nintendo just uses the last candidate if they all fail...should we abort? */
    return candidates[num_candidates - 1];
}

const char *Updater::GetBctPath(BootImageUpdateType boot_image_update_type) {
    switch (boot_image_update_type) {
        case BootImageUpdateType_Erista:
            {
                const char *candidates[] = {BctPathNx};
                return ChooseCandidatePath(candidates, sizeof(candidates) / sizeof(candidates[0]));
            }
        case BootImageUpdateType_Mariko:
            {
                const char *candidates[] = {BctPathA, BctPathNx};
                return ChooseCandidatePath(candidates, sizeof(candidates) / sizeof(candidates[0]));
            }
        default:
            std::abort();
    }
}

const char *Updater::GetPackage1Path(BootImageUpdateType boot_image_update_type) {
    switch (boot_image_update_type) {
        case BootImageUpdateType_Erista:
            {
                const char *candidates[] = {Package1PathNx};
                return ChooseCandidatePath(candidates, sizeof(candidates) / sizeof(candidates[0]));
            }
        case BootImageUpdateType_Mariko:
            {
                const char *candidates[] = {Package1PathA, Package1PathNx};
                return ChooseCandidatePath(candidates, sizeof(candidates) / sizeof(candidates[0]));
            }
        default:
            std::abort();
    }
}

const char *Updater::GetPackage2Path(BootImageUpdateType boot_image_update_type) {
    switch (boot_image_update_type) {
        case BootImageUpdateType_Erista:
            {
                const char *candidates[] = {Package2PathNx};
                return ChooseCandidatePath(candidates, sizeof(candidates) / sizeof(candidates[0]));
            }
        case BootImageUpdateType_Mariko:
            {
                const char *candidates[] = {Package2PathA, Package2PathNx};
                return ChooseCandidatePath(candidates, sizeof(candidates) / sizeof(candidates[0]));
            }
        default:
            std::abort();
    }
}