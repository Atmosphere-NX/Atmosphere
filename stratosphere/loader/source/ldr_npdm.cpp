#include <switch.h>
#include <algorithm>
#include <cstdio>
#include "ldr_npdm.hpp"
#include "ldr_registration.hpp"

static NpdmUtils::NpdmCache g_npdm_cache = {0};


Result NpdmUtils::LoadNpdm(u64 tid, NpdmInfo *out) {
    Result rc;
    
    g_npdm_cache.info = (const NpdmUtils::NpdmInfo){0};
    
    FILE *f_npdm = fopen("code:/main.npdm", "rb");
    if (f_npdm == NULL) {
        /* For generic "Couldn't open the file" error, just say the file doesn't exist. */
        return 0x202;
    }
    
    fseek(f_npdm, 0, SEEK_END);
    size_t npdm_size = ftell(f_npdm);
    fseek(f_npdm, 0, SEEK_SET);
    
    if (npdm_size > sizeof(g_npdm_cache.buffer) || fread(g_npdm_cache.buffer, 1, npdm_size, f_npdm) != npdm_size) {
        return 0x609;
    }
    
    fclose(f_npdm);
    
    rc = 0x809;
    if (npdm_size < sizeof(NpdmUtils::NpdmHeader)) {
        return rc;
    }
    
    /* For ease of access... */
    g_npdm_cache.info.header = (NpdmUtils::NpdmHeader *)(g_npdm_cache.buffer);
    NpdmInfo *info = &g_npdm_cache.info;
    
    if (info->header->magic != MAGIC_META) {
        return rc;
    }
    
    if (info->header->mmu_flags > 0xF) {
        return rc;
    }
    
    if (info->header->aci0_offset < sizeof(NpdmUtils::NpdmHeader) || info->header->aci0_size < sizeof(NpdmUtils::NpdmAci0) || info->header->aci0_offset + info->header->aci0_size > npdm_size) {
        return rc;
    }
    
    info->aci0 = (NpdmAci0 *)(g_npdm_cache.buffer + info->header->aci0_offset);
    
    if (info->aci0->magic != MAGIC_ACI0) {
        return rc;
    }
    
    if (info->aci0->fah_size > info->header->aci0_size || info->aci0->fah_offset < sizeof(NpdmUtils::NpdmAci0) || info->aci0->fah_offset + info->aci0->fah_size > info->header->aci0_size) {
        return rc;
    }
    
    info->aci0_fah = (void *)((uintptr_t)info->aci0 + info->aci0->fah_offset);
    
    if (info->aci0->sac_size > info->header->aci0_size || info->aci0->sac_offset < sizeof(NpdmUtils::NpdmAci0) || info->aci0->sac_offset + info->aci0->sac_size > info->header->aci0_size) {
        return rc;
    }
    
    info->aci0_sac = (void *)((uintptr_t)info->aci0 + info->aci0->sac_offset);
    
    if (info->aci0->kac_size > info->header->aci0_size || info->aci0->kac_offset < sizeof(NpdmUtils::NpdmAci0) || info->aci0->kac_offset + info->aci0->kac_size > info->header->aci0_size) {
        return rc;
    }
    
    info->aci0_kac = (void *)((uintptr_t)info->aci0 + info->aci0->kac_offset);
    
    if (info->header->acid_offset < sizeof(NpdmUtils::NpdmHeader) || info->header->acid_size < sizeof(NpdmUtils::NpdmAcid) || info->header->acid_offset + info->header->acid_size > npdm_size) {
        return rc;
    }
    
    info->acid = (NpdmAcid *)(g_npdm_cache.buffer + info->header->acid_offset);
    
    if (info->acid->magic != MAGIC_ACID) {
        return rc;
    }
    
    /* TODO: Check if retail flag is set if not development hardware. */
    
    if (info->acid->fac_size > info->header->acid_size || info->acid->fac_offset < sizeof(NpdmUtils::NpdmAcid) || info->acid->fac_offset + info->acid->fac_size > info->header->acid_size) {
        return rc;
    }
    
    info->acid_fac = (void *)((uintptr_t)info->acid + info->acid->fac_offset);
    
    if (info->acid->sac_size > info->header->acid_size || info->acid->sac_offset < sizeof(NpdmUtils::NpdmAcid) || info->acid->sac_offset + info->acid->sac_size > info->header->acid_size) {
        return rc;
    }
    
    info->acid_sac = (void *)((uintptr_t)info->acid + info->acid->sac_offset);
    
    if (info->acid->kac_size > info->header->acid_size || info->acid->kac_offset < sizeof(NpdmUtils::NpdmAcid) || info->acid->kac_offset + info->acid->kac_size > info->header->acid_size) {
        return rc;
    }
    
    info->acid_kac = (void *)((uintptr_t)info->acid + info->acid->kac_offset);
    
    
    /* We validated! */
    info->title_id = tid;
    *out = *info;
    rc = 0;
    
    return rc;
}