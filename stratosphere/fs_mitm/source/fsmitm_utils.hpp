#pragma once
#include <switch.h>
#include <stratosphere.hpp>

class Utils {
    public:
        static bool IsSdInitialized();
        static Result OpenSdFile(const char *fn, int flags, FsFile *out);
        static Result OpenSdFileForAtmosphere(u64 title_id, const char *fn, int flags, FsFile *out);
        static Result OpenRomFSSdFile(u64 title_id, const char *fn, int flags, FsFile *out);
        static Result OpenSdDir(const char *path, FsDir *out);
        static Result OpenSdDirForAtmosphere(u64 title_id, const char *path, FsDir *out);
        static Result OpenRomFSSdDir(u64 title_id, const char *path, FsDir *out);
        
        
        static Result OpenRomFSFile(FsFileSystem *fs, u64 title_id, const char *fn, int flags, FsFile *out);
        static Result OpenRomFSDir(FsFileSystem *fs, u64 title_id, const char *path, FsDir *out);
};