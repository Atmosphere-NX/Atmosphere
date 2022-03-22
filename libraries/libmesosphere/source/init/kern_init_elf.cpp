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
        uintptr_t rel_count  = 0;
        uintptr_t rela_count = 0;
        uintptr_t rel_ent    = 0;
        uintptr_t rela_ent   = 0;

        /* Iterate over all tags, identifying important extents. */
        for (const Dyn *cur_entry = dynamic; cur_entry->GetTag() != DT_NULL; cur_entry++) {
            switch (cur_entry->GetTag()) {
                case DT_REL:
                    dyn_rel    = base_address + cur_entry->GetPtr();
                    break;
                case DT_RELA:
                    dyn_rela   = base_address + cur_entry->GetPtr();
                    break;
                case DT_RELENT:
                    rel_ent    = cur_entry->GetValue();
                    break;
                case DT_RELAENT:
                    rela_ent   = cur_entry->GetValue();
                    break;
                case DT_RELCOUNT:
                    rel_count  = cur_entry->GetValue();
                    break;
                case DT_RELACOUNT:
                    rela_count = cur_entry->GetValue();
                    break;
            }
        }

        /* Apply all Rel relocations */
        for (size_t i = 0; i < rel_count; i++) {
            const auto &rel = *reinterpret_cast<const Elf::Rel *>(dyn_rel + rel_ent * i);

            /* Only allow architecture-specific relocations. */
            while (rel.GetType() != R_ARCHITECTURE_RELATIVE) { /* ... */ }

            /* Apply the relocation. */
            Elf::Addr *target_address = reinterpret_cast<Elf::Addr *>(base_address + rel.GetOffset());
            *target_address += base_address;
        }

        /* Apply all Rela relocations. */
        for (size_t i = 0; i < rela_count; i++) {
            const auto &rela = *reinterpret_cast<const Elf::Rela *>(dyn_rela + rela_ent * i);

            /* Only allow architecture-specific relocations. */
            while (rela.GetType() != R_ARCHITECTURE_RELATIVE) { /* ... */ }

            /* Apply the relocation. */
            Elf::Addr *target_address = reinterpret_cast<Elf::Addr *>(base_address + rela.GetOffset());
            *target_address = base_address + rela.GetAddend();
        }
    }

    void CallInitArrayFuncs(uintptr_t init_array_start, uintptr_t init_array_end) {
        for (uintptr_t cur_entry = init_array_start; cur_entry < init_array_end; cur_entry += sizeof(void *)) {
            (*(void (**)())(cur_entry))();
        }
    }

}