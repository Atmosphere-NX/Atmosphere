/*
 * Copyright (c) Atmosphère-NX
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
#include <mesosphere.hpp>

namespace ams::kern::init::Elf {

    /* API to apply relocations or call init array. */
    void ApplyRelocations(uintptr_t base_address, const Dyn *dynamic) {
        uintptr_t dyn_rel    = 0;
        uintptr_t dyn_rela   = 0;
        uintptr_t dyn_relr   = 0;
        uintptr_t rel_count  = 0;
        uintptr_t rela_count = 0;
        uintptr_t relr_sz    = 0;
        uintptr_t rel_ent    = 0;
        uintptr_t rela_ent   = 0;
        uintptr_t relr_ent   = 0;

        /* Iterate over all tags, identifying important extents. */
        for (const Dyn *cur_entry = dynamic; cur_entry->GetTag() != DT_NULL; cur_entry++) {
            switch (cur_entry->GetTag()) {
                case DT_REL:
                    dyn_rel    = base_address + cur_entry->GetPtr();
                    break;
                case DT_RELA:
                    dyn_rela   = base_address + cur_entry->GetPtr();
                    break;
                case DT_RELR:
                    dyn_relr   = base_address + cur_entry->GetPtr();
                    break;
                case DT_RELENT:
                    rel_ent    = cur_entry->GetValue();
                    break;
                case DT_RELAENT:
                    rela_ent   = cur_entry->GetValue();
                    break;
                case DT_RELRENT:
                    relr_ent   = cur_entry->GetValue();
                    break;
                case DT_RELCOUNT:
                    rel_count  = cur_entry->GetValue();
                    break;
                case DT_RELACOUNT:
                    rela_count = cur_entry->GetValue();
                    break;
                case DT_RELRSZ:
                    relr_sz    = cur_entry->GetValue();
                    break;
            }
        }

        /* Apply all Rel relocations */
        if (rel_count > 0) {
            /* Check that the rel relocations are applyable. */
            MESOSPHERE_INIT_ABORT_UNLESS(dyn_rel != 0);
            MESOSPHERE_INIT_ABORT_UNLESS(rel_ent == sizeof(Elf::Rel));

            for (size_t i = 0; i < rel_count; ++i) {
                const auto &rel = reinterpret_cast<const Elf::Rel *>(dyn_rel)[i];

                /* Only allow architecture-specific relocations. */
                while (rel.GetType() != R_ARCHITECTURE_RELATIVE) { /* ... */ }

                /* Apply the relocation. */
                Elf::Addr *target_address = reinterpret_cast<Elf::Addr *>(base_address + rel.GetOffset());
                *target_address += base_address;
            }
        }

        /* Apply all Rela relocations. */
        if (rela_count > 0) {
            /* Check that the rela relocations are applyable. */
            MESOSPHERE_INIT_ABORT_UNLESS(dyn_rela != 0);
            MESOSPHERE_INIT_ABORT_UNLESS(rela_ent == sizeof(Elf::Rela));

            for (size_t i = 0; i < rela_count; ++i) {
                const auto &rela = reinterpret_cast<const Elf::Rela *>(dyn_rela)[i];

                /* Only allow architecture-specific relocations. */
                while (rela.GetType() != R_ARCHITECTURE_RELATIVE) { /* ... */ }

                /* Apply the relocation. */
                Elf::Addr *target_address = reinterpret_cast<Elf::Addr *>(base_address + rela.GetOffset());
                *target_address = base_address + rela.GetAddend();
            }
        }

        /* Apply all Relr relocations. */
        if (relr_sz >= sizeof(Elf::Relr)) {
            /* Check that the relr relocations are applyable. */
            MESOSPHERE_INIT_ABORT_UNLESS(dyn_relr != 0);
            MESOSPHERE_INIT_ABORT_UNLESS(relr_ent == sizeof(Elf::Relr));

            const size_t relr_count = relr_sz / sizeof(Elf::Relr);

            Elf::Addr *where = nullptr;
            for (size_t i = 0; i < relr_count; ++i) {
                const auto &relr = reinterpret_cast<const Elf::Relr *>(dyn_relr)[i];

                if (relr.IsLocation()) {
                    /* Update location. */
                    where = reinterpret_cast<Elf::Addr *>(base_address + relr.GetLocation());

                    /* Apply the relocation. */
                    *(where++) += base_address;
                } else {
                    /* Get the bitmap. */
                    u64 bitmap = relr.GetBitmap();

                    /* Apply all relocations. */
                    while (bitmap != 0) {
                        const u64 next = util::CountTrailingZeros(bitmap);
                        bitmap &= ~(static_cast<u64>(1) << next);
                        where[next] += base_address;
                    }

                    /* Advance. */
                    where += BITSIZEOF(bitmap) - 1;
                }
            }
        }
    }

    void CallInitArrayFuncs(uintptr_t init_array_start, uintptr_t init_array_end) {
        for (uintptr_t cur_entry = init_array_start; cur_entry < init_array_end; cur_entry += sizeof(void *)) {
            (*(void (**)())(cur_entry))();
        }
    }

}