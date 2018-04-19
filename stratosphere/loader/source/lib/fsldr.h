/**
 * @file fsldr.h
 * @brief FilesystemProxy-ForLoader (fsp-ldr) service IPC wrapper.
 * @author SciresM
 * @copyright libnx Authors
 */
#pragma once
#include <switch/types.h>
#include <switch/services/sm.h>
#include <switch/services/fs.h>

Result fsldrInitialize(void);
void fsldrExit(void);

Result fsldrOpenCodeFileSystem(u64 tid, const char *path, FsFileSystem* out);
Result fsldrIsArchivedProgram(u64 pid, bool *out);
Result fsldrSetCurrentProcess();