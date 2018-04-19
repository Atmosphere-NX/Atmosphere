#pragma once
#include <switch.h>

#include "ldr_registration.hpp"

class ContentManagement {
    public:
        Result MountCode(u64 tid, FsStorageId sid);
        Result UnmountCode();
        Result MountCodeForTidSid(Registration::TidSid *tid_sid);

        Result GetContentPath(char *out_path, u64 tid, FsStorageId sid);
        Result SetContentPath(char *path, u64 tid, FsStorageId sid);
        Result GetContentPathForTidSid(char *out_path, Registration::TidSid *tid_sid);
        Result SetContentPathForTidSid(char *path, Registration::TidSid *tid_sid);
};
