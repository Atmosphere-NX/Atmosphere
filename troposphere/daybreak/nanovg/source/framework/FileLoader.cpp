/*
** Sample Framework for deko3d Applications
**   FileLoader.cpp: Helpers for loading data from the filesystem directly into GPU memory
*/
#include "FileLoader.h"

CMemPool::Handle LoadFile(CMemPool& pool, const char* path, uint32_t alignment)
{
    FILE *f = fopen(path, "rb");
    if (!f) return nullptr;

    fseek(f, 0, SEEK_END);
    uint32_t fsize = ftell(f);
    rewind(f);

    CMemPool::Handle mem = pool.allocate(fsize, alignment);
    if (!mem)
    {
        fclose(f);
        return nullptr;
    }

    fread(mem.getCpuAddr(), fsize, 1, f);
    fclose(f);

    return mem;
}
