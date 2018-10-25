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
        static void RefreshConfigurationData();
        static void TryMountSdCard();
        
        static bool ShouldReplaceWithHBL(u64 tid);
        static bool ShouldOverrideContents(u64 tid);

        /* SetExternalContentSource extension */
        class ExternalContentSource {
            public:
                static void GenerateMountpointName(u64 tid, char *out, size_t max_length);

                ExternalContentSource(u64 tid, const char *mountpoint);
                ~ExternalContentSource();

                ExternalContentSource(const ExternalContentSource &other) = delete;
                ExternalContentSource(ExternalContentSource &&other) = delete;
                ExternalContentSource &operator=(const ExternalContentSource &other) = delete;
                ExternalContentSource &operator=(ExternalContentSource &&other) = delete;

                const u64 tid;
                char mountpoint[32];
        };
        static ExternalContentSource *GetExternalContentSource(u64 tid); /* returns nullptr if no ECS is set */
        static Result SetExternalContentSource(u64 tid, FsFileSystem filesystem); /* takes ownership of filesystem */
        static void ClearExternalContentSource(u64 tid);
};
