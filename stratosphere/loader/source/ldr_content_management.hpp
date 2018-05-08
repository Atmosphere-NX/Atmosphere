#pragma once
#include <switch.h>

#include "ldr_registration.hpp"

class ContentManagement {
    public:
        static Result MountCode(u64 tid, FsStorageId sid);
        static Result UnmountCode();
        static Result MountCodeForTidSid(Registration::TidSid *tid_sid);

        static Result ResolveContentPath(char *out_path, u64 tid, FsStorageId sid);
        static Result RedirectContentPath(const char *path, u64 tid, FsStorageId sid);
        static Result ResolveContentPathForTidSid(char *out_path, Registration::TidSid *tid_sid);
        static Result RedirectContentPathForTidSid(const char *path, Registration::TidSid *tid_sid);
        
        static bool HasCreatedTitle(u64 tid);
        static void SetCreatedTitle(u64 tid);
        static void TryMountSdCard();
};
