#pragma once
#include <switch.h>

#include "ldr_registration.hpp"

Result GetContentPath(char *out_path, u64 tid, FsStorageId sid);
Result GetContentPathForTidSid(char *out_path, Registration::TidSid *tid_sid);