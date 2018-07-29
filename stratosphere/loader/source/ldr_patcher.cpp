#include <cstdlib>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <dirent.h>

#include <switch.h>
#include "ldr_patcher.hpp"

#define IPS_MAGIC "PATCH"
#define IPS_TAIL "EOF"

#define IPS32_MAGIC "IPS32"
#define IPS32_TAIL "EEOF"

static inline u8 HexNybbleToU8(const char nybble) {
    if ('0' <= nybble && nybble <= '9') {
        return nybble - '0';
    } else if ('a' <= nybble && nybble <= 'f') {
        return nybble - 'a';
    } else {
        return nybble - 'A';
    }
}

static bool MatchesBuildId(const char *name, size_t name_len, const u8 *build_id) {
    /* Validate name is hex build id. */
    for (unsigned int i = 0; i < name_len - 4; i++) {
        if (!(('0' <= name[i] && name[i] <= '9') ||
            ('a' <= name[i] && name[i] <= 'f') ||
            ('A' <= name[i] && name[i] <= 'F'))) {
                return false;
            }
    }
    
    /* Read build id from name. */
    u8 build_id_from_name[0x20] = {0};
    for (unsigned int name_ofs = 0, id_ofs = 0; name_ofs < name_len - 4; id_ofs++) {
        build_id_from_name[id_ofs] |= HexNybbleToU8(name[name_ofs++]) << 4;
        build_id_from_name[id_ofs] |= HexNybbleToU8(name[name_ofs++]);
    }
    
    return memcmp(build_id, build_id_from_name, sizeof(build_id_from_name)) == 0;
}

static inline void PatchNsoByte(u8 *nso, size_t max_size, size_t offset, u8 val) {
    /* TODO: Be smarter about this. */
    if (sizeof(NsoUtils::NsoHeader) <= offset && offset < max_size) {
        nso[offset - sizeof(NsoUtils::NsoHeader)] = val;
    }
}

static void ApplyIpsPatch(u8 *mapped_nso, size_t mapped_size, FILE *ips) {
    /* TODO */
}

static void ApplyIps32Patch(u8 *mapped_nso, size_t mapped_size, FILE *ips) {
    /* TODO */
}

void PatchUtils::ApplyPatches(u64 title_id, const NsoUtils::NsoHeader *header, u8 *mapped_nso, size_t mapped_size) {
    /* Inspect all patches from /<title>/exefs/patches/<*>/<*>.ips. */
    char path[FS_MAX_PATH+1] = {0};
    snprintf(path, sizeof(path) - 1, "sdmc:/atmosphere/titles/%016lx/exefs/patches", title_id);
    DIR *patches_dir = opendir(path);
    struct dirent *pdir_ent;
    if (patches_dir != NULL) {
        /* Iterate over the patches directory to find patch subdirectories. */
        while ((pdir_ent = readdir(patches_dir)) != NULL) {
            if (strcmp(pdir_ent->d_name, ".") == 0 || strcmp(pdir_ent->d_name, "..") == 0) {
                continue;
            }
            snprintf(path, sizeof(path) - 1, "sdmc:/atmosphere/titles/%016lx/exefs/patches/%s", title_id, pdir_ent->d_name);
            DIR *patch_dir = opendir(path);
            struct dirent *ent;
            if (patch_dir != NULL) {
                /* Iterate over the patch subdirectory to find .ips patches. */
                while ((ent = readdir(patch_dir)) != NULL) {
                    if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) {
                        continue;
                    }
                    size_t name_len = strlen(ent->d_name);
                    if ((4 < name_len && name_len <= 0x44) && ((name_len & 1) == 0) && strcmp(ent->d_name + name_len - 4, ".ips") == 0 && MatchesBuildId(ent->d_name, name_len, header->build_id)) {
                        snprintf(path, sizeof(path) - 1, "sdmc:/atmosphere/titles/%016lx/exefs/patches/%s/%s", title_id, pdir_ent->d_name, ent->d_name);
                        FILE *f_ips = fopen(path, "rb");
                        if (f_ips != NULL) {
                            u8 header[5];
                            if (fread(header, 5, 1, f_ips) == 1) {
                                if (memcmp(header, IPS_MAGIC, 5) == 0) {
                                    ApplyIpsPatch(mapped_nso, mapped_size, f_ips);
                                } else if (memcmp(header, IPS32_MAGIC, 5) == 0) {
                                    ApplyIps32Patch(mapped_nso, mapped_size, f_ips);
                                }
                            }
                            fclose(f_ips);
                        }
                    }
                }
                closedir(patch_dir);
            }
        }
        closedir(patches_dir);
    }
}