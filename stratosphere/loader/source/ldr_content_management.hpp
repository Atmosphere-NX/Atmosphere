/*
 * Copyright (c) 2018 Atmosphère-NX
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
#include <switch.h>

#include "ldr_registration.hpp"

class ContentManagement {
    public:
        static Result MountCode(u64 tid, FsStorageId sid);
        static Result MountCodeNspOnSd(u64 tid);
        static void TryMountHblNspOnSd();
        static Result UnmountCode();
        static Result MountCodeForTidSid(Registration::TidSid *tid_sid);

        static Result ResolveContentPath(char *out_path, u64 tid, FsStorageId sid);
        static Result RedirectContentPath(const char *path, u64 tid, FsStorageId sid);
        static Result ResolveContentPathForTidSid(char *out_path, Registration::TidSid *tid_sid);
        static Result RedirectContentPathForTidSid(const char *path, Registration::TidSid *tid_sid);
        
        static bool HasCreatedTitle(u64 tid);
        static void SetCreatedTitle(u64 tid);
        static void LoadConfiguration(FILE *config);
        static void TryMountSdCard();
        
        static bool ShouldReplaceWithHBL(u64 tid);
        static bool ShouldOverrideContents();
};
