/*
 * Copyright (c) 2018 Atmosphère-NX
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
 
#include <switch.h>
#include <cstring>
#include <stratosphere.hpp>

#include "fsmitm_boot0storage.hpp"

static HosMutex g_boot0_mutex;
static u8 g_boot0_bct_buffer[Boot0Storage::BctEndOffset];

bool Boot0Storage::AllowWrites() {
    return this->title_id < 0x0100000000001000ULL;
}

bool Boot0Storage::CanModifyBctPubks() {
    return this->title_id != 0x010000000000001FULL;
}

Result Boot0Storage::Read(void *_buffer, size_t size, u64 offset) {
    std::scoped_lock<HosMutex> lk{g_boot0_mutex};
            
    return Base::Read(_buffer, size, offset);
}

Result Boot0Storage::Write(void *_buffer, size_t size, u64 offset) {
    std::scoped_lock<HosMutex> lk{g_boot0_mutex};
    
    if (!AllowWrites()) {
        return 0x313802;
    }
    
    /* We care about protecting autorcm from NS. */
    if (CanModifyBctPubks() || offset >= BctEndOffset || (offset + BctSize >= BctEndOffset && offset % BctSize >= BctPubkEnd)) {
        return Base::Write(_buffer, size, offset);
    }
    
    Result rc = 0;
    u8 *buffer = static_cast<u8 *>(_buffer);
    
    /* First, let's deal with the data past the end. */
    if (offset + size >= BctEndOffset) {
        const u64 diff = BctEndOffset - offset;
        if (R_FAILED((rc = ProxyStorage::Write(buffer + diff, size - diff, BctEndOffset)))) {
            return rc;
        }
        size -= diff;
    }
    
    /* Read in the current BCT region. */
    if (R_FAILED((rc = ProxyStorage::Read(g_boot0_bct_buffer, BctEndOffset, 0)))) {
        return rc;
    }
    
    /* Update the bct buffer. */
    for (u64 cur_ofs = offset; cur_ofs < BctEndOffset && cur_ofs < offset + size; cur_ofs++) {
        const u64 cur_bct_rel_ofs = cur_ofs % BctSize;
        if (cur_bct_rel_ofs < BctPubkStart || BctPubkEnd <= cur_bct_rel_ofs) {
            g_boot0_bct_buffer[cur_ofs] = buffer[cur_ofs - offset];
        }
    }
            
    return ProxyStorage::Write(g_boot0_bct_buffer, BctEndOffset, 0);
}
