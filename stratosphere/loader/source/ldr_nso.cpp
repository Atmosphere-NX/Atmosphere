#include <switch.h>
#include <algorithm>
#include <cstdio>
#include "ldr_nso.hpp"

static NsoUtils::NsoHeader g_nso_headers[NSO_NUM_MAX] = {0};
static bool g_nso_present[NSO_NUM_MAX] = {0};

void NsoUtils::GetNsoCodePath(char *content_path, unsigned int index) {
    std::fill(content_path, content_path + FS_MAX_PATH, 0);
    snprintf(content_path, FS_MAX_PATH, "code:/%s", NsoUtils::GetNsoFileName(index));
}

void NsoUtils::GetNsoSdPath(char *content_path, u64 title_id, unsigned int index) {  
    std::fill(content_path, content_path + FS_MAX_PATH, 0);
    snprintf(content_path, FS_MAX_PATH, "sdmc:/atmosphere/titles/%016lx/exefs/%s", title_id, NsoUtils::GetNsoFileName(index));
}

bool NsoUtils::IsNsoPresent(unsigned int index) {
    return g_nso_present[index];
}

Result NsoUtils::LoadNsoHeaders(u64 title_id) {
    char nso_path[FS_MAX_PATH] = {0};
    FILE *f_nso;
    
    /* Zero out the cache. */
    std::fill(g_nso_present, g_nso_present + NSO_NUM_MAX, false);
    std::fill(g_nso_headers, g_nso_headers + NSO_NUM_MAX, (const NsoUtils::NsoHeader &){0});
    
    for (unsigned int i = 0; i < NSO_NUM_MAX; i++) {
        GetNsoSdPath(nso_path, title_id, i);
        if ((f_nso = fopen(nso_path, "rb")) != NULL) {
            if (fread(&g_nso_headers[i], sizeof(NsoUtils::NsoHeader), 1, f_nso) != sizeof(NsoUtils::NsoHeader)) {
                return 0xA09;
            }
            g_nso_present[i] = true;
            fclose(f_nso);
            continue;
        }
        GetNsoCodePath(nso_path, i);
        if ((f_nso = fopen(nso_path, "rb")) != NULL) {
            if (fread(&g_nso_headers[i], sizeof(NsoUtils::NsoHeader), 1, f_nso) != sizeof(NsoUtils::NsoHeader)) {
                return 0xA09;
            }
            g_nso_present[i] = true;
            fclose(f_nso);
            continue;
        }
        if (1 < i && i < 12) {
            /* If we failed to open a subsdk, there are no more subsdks. */
            i = 11;
        }
    }
    
    return 0x0;
}

Result NsoUtils::ValidateNsoLoadSet() {
    /* We *must* have a "main" NSO. */
    if (!g_nso_present[1]) {
        return 0xA09;
    }
    
    /* Behavior switches depending on whether we have an rtld. */
    if (g_nso_present[0]) {
         /* If we have an rtld, dst offset for .text must be 0 for all other NSOs. */
        for (unsigned int i = 0; i < NSO_NUM_MAX; i++) {
            if (g_nso_present[i] && g_nso_headers[i].segments[0].dst_offset != 0) {
                return 0xA09;
            }
        }
    } else {
        /* If we don't have an rtld, we must ONLY have a main. */
        for (unsigned int i = 2; i < NSO_NUM_MAX; i++) {
            if (g_nso_present[i]) {
                return 0xA09;
            }
        }
        /* That main's .text must be at dst_offset 0. */
        if (g_nso_headers[1].segments[0].dst_offset != 0) {
            return 0xA09;
        }
    }
    
    return 0x0;
}