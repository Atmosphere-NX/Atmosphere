/*
 * Copyright (c) 2018-2020 Atmosphère-NX
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
#include "fsmitm_boot0storage.hpp"

namespace ams::mitm::fs {

    using namespace ams::fs;

    namespace {

        os::Mutex g_boot0_access_mutex(false);
        u8 g_boot0_bct_buffer[Boot0Storage::BctEndOffset];

    }

    bool Boot0Storage::CanModifyBctPublicKey() {
        if (exosphere::IsRcmBugPatched()) {
            /* RCM bug patched. */
            /* Only allow NS to update the BCT pubks. */
            /* AutoRCM on a patched unit will cause a brick, so homebrew should NOT be allowed to write. */
            return this->client_info.program_id == ncm::SystemProgramId::Ns;
        } else {
            /* RCM bug unpatched. */
            /* Allow homebrew but not NS to update the BCT pubks. */
            return this->client_info.override_status.IsHbl();
        }
    }

    Result Boot0Storage::Read(s64 offset, void *_buffer, size_t size) {
        std::scoped_lock lk{g_boot0_access_mutex};

        /* Check if we have nothing to do. */
        if (size == 0) {
            return ResultSuccess();
        }

        return Base::Read(offset, _buffer, size);
    }

    Result Boot0Storage::Write(s64 offset, const void *_buffer, size_t size) {
        std::scoped_lock lk{g_boot0_access_mutex};

        const u8 *buffer = static_cast<const u8 *>(_buffer);

        /* Check if we have nothing to do. */
        if (size == 0) {
            return ResultSuccess();
        }

        /* Protect the EKS region from writes. */
        if (offset <= EksStart) {
            if (offset + size < EksStart) {
                /* Fall through, no need to do anything here. */
            } else {
                if (offset + size > EksEnd) {
                    /* Perform portion of write falling past end of keyblobs. */
                    const s64 diff = EksEnd - offset;
                    R_TRY(Base::Write(EksEnd, buffer + diff, size - diff));
                }
                /* Adjust size to avoid writing end of data. */
                size = EksStart - offset;
            }
        } else if (offset < EksEnd) {
            if (offset + size <= EksEnd) {
                /* Ignore writes falling strictly within the region. */
                return ResultSuccess();
            } else {
                /* Only write past the end of the region. */
                const s64 diff = EksEnd - offset;
                buffer += diff;
                size   -= diff;
                offset = EksEnd;
            }
        }

        /* If we have nothing to write, succeed immediately. */
        if (size == 0) {
            return ResultSuccess();
        }

        /* We want to protect AutoRCM from NS on ipatched units. If we can modify bct pubks or we're not touching any of them, proceed. */
        if (this->CanModifyBctPublicKey() || offset >= BctEndOffset || (util::AlignUp(offset, BctSize) >= BctEndOffset && (offset % BctSize) >= BctPubkEnd)) {
            return Base::Write(offset, buffer, size);
        }

        /* Handle any data written past the end of the pubk region. */
        if (offset + size > BctEndOffset) {
            const u64 diff = BctEndOffset - offset;
            R_TRY(Base::Write(BctEndOffset, buffer + diff, size - diff));
            size = diff;
        }

        /* Read in the current BCT region. */
        R_TRY(Base::Read(0, g_boot0_bct_buffer, BctEndOffset));

        /* Update the bct buffer. */
        for (u64 cur_offset = offset; cur_offset < BctEndOffset && cur_offset < offset + size; cur_offset++) {
            const u64 cur_bct_relative_ofs = cur_offset % BctSize;
            if (cur_bct_relative_ofs < BctPubkStart || BctPubkEnd <= cur_bct_relative_ofs) {
                g_boot0_bct_buffer[cur_offset] = buffer[cur_offset - offset];
            }
        }

        return Base::Write(0, g_boot0_bct_buffer, BctEndOffset);
    }

}
