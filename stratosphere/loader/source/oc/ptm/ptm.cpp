/*
 * Copyright (C) Switch-OC-Suite
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "ptm.hpp"

namespace ams::ldr::oc::ptm {

Result CpuPtmBoost(perf_conf_entry* entry) {
    if (!C.commonCpuBoostClock)
        R_SUCCEED();

    u32 cpuPtmBoostNew = C.commonCpuBoostClock * 1000;

    PATCH_OFFSET(&(entry->cpu_freq_1), cpuPtmBoostNew);
    PATCH_OFFSET(&(entry->cpu_freq_2), cpuPtmBoostNew);

    R_SUCCEED();
}

Result MemPtm(perf_conf_entry* entry) {
    PATCH_OFFSET(&(entry->emc_freq_1), memPtmLimit);
    PATCH_OFFSET(&(entry->emc_freq_2), memPtmLimit);

    R_SUCCEED();
}

bool PtmEntryIsValid(perf_conf_entry* entry) {
    return (entry->cpu_freq_1 == entry->cpu_freq_2 &&
            entry->gpu_freq_1 == entry->gpu_freq_2 &&
            entry->emc_freq_1 == entry->emc_freq_2);
}

bool PtmTablePatternFn(u32* ptr) {
    perf_conf_entry* entry = reinterpret_cast<perf_conf_entry *>(ptr);
    if (!PtmEntryIsValid(entry))
        return false;

    return entry->cpu_freq_1 == cpuPtmDefault;
}

void Patch(uintptr_t mapped_nso, size_t nso_size) {
    perf_conf_entry* confTable = nullptr;
    for (uintptr_t ptr = mapped_nso;
         ptr <= mapped_nso + nso_size - sizeof(perf_conf_entry) * entryCnt;
         ptr += sizeof(u32))
    {
        u32* ptr32 = reinterpret_cast<u32 *>(ptr);
        if (PtmTablePatternFn(ptr32)) {
            confTable = reinterpret_cast<perf_conf_entry *>(ptr);
            break;
        }
    }

    if (!confTable) {
        CRASH("confTable not found!");
    }

    PatcherEntry<perf_conf_entry> cpuPtmBoostPatch = { "CPU Ptm Boost", &CpuPtmBoost, 2, };
    PatcherEntry<perf_conf_entry> memPtmPatch = { "MEM Ptm", &MemPtm, 16, };

    #ifdef ATMOSPHERE_IS_STRATOSPHERE
    bool isMariko = (spl::GetSocType() == spl::SocType_Mariko);
    #else
    bool isMariko = true;
    #endif


    for (u32 i = 0; i < entryCnt; i++) {
        perf_conf_entry* entry = confTable + i;

        if (!PtmEntryIsValid(entry)) {
            LOGGING("@%p", &entry);
            CRASH("Invalid ptm confTable entry");
        }

        switch (entry->cpu_freq_1) {
            case cpuPtmBoost:
                cpuPtmBoostPatch.Apply(entry);
                break;
            case cpuPtmDefault:
            case cpuPtmDevOC:
                break;
            default:
                LOGGING("%u (0x%08x) @%p", entry->cpu_freq_1, entry->conf_id, &(entry->cpu_freq_1));
                CRASH("Unknown CPU Freq");
        }

        switch (entry->emc_freq_1) {
            case memPtmLimit:
            case memPtmAlt:
            case memPtmClamp:
                if (isMariko) {
                    memPtmPatch.Apply(entry);
                }
                break;
            default:
                LOGGING("%u (0x%08x) @%p", entry->emc_freq_1, entry->conf_id, &(entry->emc_freq_2));
                CRASH("Unknown MEM Freq");
        }
    }

    LOGGING("%s Count: %zu", cpuPtmBoostPatch.description, cpuPtmBoostPatch.patched_count);
    if (R_FAILED(cpuPtmBoostPatch.CheckResult()))
        CRASH(cpuPtmBoostPatch.description);

    if (isMariko) {
        LOGGING("%s Count: %zu", memPtmPatch.description, memPtmPatch.patched_count);
        if (R_FAILED(memPtmPatch.CheckResult()))
            CRASH(memPtmPatch.description);
    }
}

}
