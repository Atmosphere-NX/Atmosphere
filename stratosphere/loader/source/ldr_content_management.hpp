#pragma once
#include <switch.h>

#include "ldr_registration.hpp"

class ContentManagement {
    public:
        static Result MountCode(u64 tid, FsStorageId sid);
        static Result UnmountCode();
        static Result MountCodeForTidSid(Registration::TidSid *tid_sid);

        static Result GetContentPath(char *out_path, u64 tid, FsStorageId sid);
        static Result SetContentPath(const char *path, u64 tid, FsStorageId sid);
        static Result GetContentPathForTidSid(char *out_path, Registration::TidSid *tid_sid);
        static Result SetContentPathForTidSid(const char *path, Registration::TidSid *tid_sid);
};
