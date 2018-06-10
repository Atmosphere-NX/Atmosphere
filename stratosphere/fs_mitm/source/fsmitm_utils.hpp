#pragma once
#include <switch.h>
#include <stratosphere.hpp>

class Utils {
    public:
        static Result OpenSdFile(const char *fn, int flags, FsFile *out);
};