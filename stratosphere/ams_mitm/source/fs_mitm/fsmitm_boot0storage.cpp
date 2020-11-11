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
#include <stratosphere.hpp>
#include "fsmitm_boot0storage.hpp"

namespace ams::mitm::fs {

    using namespace ams::fs;

    namespace {

        os::SdkMutex g_boot0_access_mutex;
        constinit bool g_custom_public_key = false;
        u8 g_boot0_bct_buffer[Boot0Storage::BctEndOffset];

        /* Recognize special public key (https://gist.github.com/SciresM/16b63ac1d80494522bdba2c57995257c). */
        /* P = 19 */
        /* Q = 1696986749729493925354392349339746171297507422986462747526968361144447230710192316397327889522451749459854070558277878297255552508603806832852079596337539247651161831569525505882103311631577368514276343192042634740927726070847704397913856975832811679847928433261678072951551065705680482548543833651752439700272736498378724153330763357721354498194000536297732323628263256733931353143625854828275237159155585342783077681713929284136658773985266864804093157854331138230313706015557050002740810464618031715670281442110238274404626065924786185264268216336867948322976979393032640085259926883014490947373494538254895109731 */
        /* N = 0xFF696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696969696959 */
        /* E = 0x10001 */
        /* D = 6512128715229088976470211610075969347035078304643231077895577077900787352712063823560162578441773733649014439616165727455431015055675770987914713980812453585413988983206576233689754710500864883529402371292948326392791238474661859182717295176679567362482790015587820446999760239570255254879359445627372805817473978644067558931078225451477635089763009580492462097185005355990612929951162366081631888011031830742459571000203341001926135389508196521518687349554188686396554248868403128728646457247197407637887195043486221295751496667162366700477934591694110831002874992896076061627516220934290742867397720103040314639313 */

        constexpr const u8 CustomPublicKey[0x100] = {
            0x59, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69,
            0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69,
            0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69,
            0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69,
            0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69,
            0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69,
            0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69,
            0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69,
            0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69,
            0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69,
            0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69,
            0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69,
            0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69,
            0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69,
            0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69,
            0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0xFF,
        };

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
        AMS_ABORT_UNLESS(!g_custom_public_key);

        /* Check if we have nothing to do. */
        R_SUCCEED_IF(size == 0);

        return Base::Read(offset, _buffer, size);
    }

    Result Boot0Storage::Write(s64 offset, const void *_buffer, size_t size) {
        std::scoped_lock lk{g_boot0_access_mutex};
        AMS_ABORT_UNLESS(!g_custom_public_key);

        const u8 *buffer = static_cast<const u8 *>(_buffer);

        /* Check if we have nothing to do. */
        R_SUCCEED_IF(size == 0);

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
            /* Ignore writes falling strictly within the region. */
            R_SUCCEED_IF(offset + size <= EksEnd);

            /* Only write past the end of the region. */
            const s64 diff = EksEnd - offset;
            buffer += diff;
            size   -= diff;
            offset = EksEnd;
        }

        /* If we have nothing to write, succeed immediately. */
        R_SUCCEED_IF(size == 0);

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

    CustomPublicKeyBoot0Storage::CustomPublicKeyBoot0Storage(FsStorage &s, const sm::MitmProcessInfo &c, spl::SocType soc) : Base(s), client_info(c), soc_type(soc) {
        std::scoped_lock lk{g_boot0_access_mutex};

        /* We're custom public key. */
        g_custom_public_key = true;
    }

    Result CustomPublicKeyBoot0Storage::Read(s64 offset, void *_buffer, size_t size) {
        std::scoped_lock lk{g_boot0_access_mutex};
        AMS_ABORT_UNLESS(g_custom_public_key);

        /* Check if we have nothing to do. */
        R_SUCCEED_IF(size == 0);

        u8 *buffer = static_cast<u8 *>(_buffer);

        /* Check if we're reading the first BCTs for NS. */
        /* If we are, we want to lie about the contents of BCT0/1 so that they validate. */
        if (offset < 0x8000 && this->client_info.program_id == ncm::SystemProgramId::Ns) {
            R_TRY(Base::Read(0, g_boot0_bct_buffer, Boot0Storage::BctEndOffset));

            /* Determine the readable size. */
            const size_t readable_bct01_size = std::min<size_t>(0x8000, offset + size) - offset;
            std::memcpy(buffer, g_boot0_bct_buffer + 0x8000 + offset, readable_bct01_size);

            /* Advance. */
            buffer += readable_bct01_size;
            offset += readable_bct01_size;
            size   -= readable_bct01_size;
        }

        /* Check if we have nothing to do. */
        R_SUCCEED_IF(size == 0);

        /* Perform whatever remains of the read. */
        return Base::Read(offset, buffer, size);
    }

    Result CustomPublicKeyBoot0Storage::Write(s64 offset, const void *_buffer, size_t size) {
        std::scoped_lock lk{g_boot0_access_mutex};
        AMS_ABORT_UNLESS(g_custom_public_key);

        const u8 *buffer = static_cast<const u8 *>(_buffer);

        /* Check if we have nothing to do. */
        R_SUCCEED_IF(size == 0);

        /* Drop writes to the first BCTs. */
        if (offset < 0x8000) {
            /* Determine the writable size. */
            const size_t writable_bct01_size = std::min<size_t>(0x8000, offset + size) - offset;

            /* Advance. */
            buffer += writable_bct01_size;
            offset += writable_bct01_size;
            size   -= writable_bct01_size;
        }

        /* Check if we have nothing to do. */
        R_SUCCEED_IF(size == 0);

        /* Similarly, we want to drop writes to the end of boot0, where custom bootloader lives. */
        R_SUCCEED_IF(offset >= 0x380000);
        if (offset + size >= 0x380000) {
            size = 0x380000 - offset;
        }

        /* On erista, we want to protect the EKS region. */
        if (this->soc_type == spl::SocType_Erista) {
            if (offset <= Boot0Storage::EksStart) {
                if (offset + size < Boot0Storage::EksStart) {
                    /* Fall through, no need to do anything here. */
                } else {
                    if (offset + size > Boot0Storage::EksEnd) {
                        /* Perform portion of write falling past end of keyblobs. */
                        const s64 diff = Boot0Storage::EksEnd - offset;
                        R_TRY(Base::Write(Boot0Storage::EksEnd, buffer + diff, size - diff));
                    }
                    /* Adjust size to avoid writing end of data. */
                    size = Boot0Storage::EksStart - offset;
                }
            } else if (offset < Boot0Storage::EksEnd) {
                /* Ignore writes falling strictly within the region. */
                R_SUCCEED_IF(offset + size <= Boot0Storage::EksEnd);

                /* Only write past the end of the region. */
                const s64 diff = Boot0Storage::EksEnd - offset;
                buffer += diff;
                size   -= diff;
                offset = Boot0Storage::EksEnd;
            }
        }

        /* If we have nothing to write, succeed immediately. */
        R_SUCCEED_IF(size == 0);

        /* Perform whatever remains of the write. */
        return Base::Write(offset, buffer, size);
    }

    bool DetectBoot0CustomPublicKey(::FsStorage &storage) {
        /* Determine public key offset. */
        const size_t bct_pubk_offset = spl::GetSocType() == spl::SocType_Mariko ? 0x10 : 0x210;

        u8 work_buffer[0x400];

        /* Read BCT-Normal-Main. */
        R_ABORT_UNLESS(::fsStorageRead(std::addressof(storage), 0, work_buffer, sizeof(work_buffer)));

        /* Check for custom public key. */
        if (std::memcmp(work_buffer + bct_pubk_offset, CustomPublicKey, sizeof(CustomPublicKey)) != 0) {
            return false;
        }

        /* Read BCT-Safe-Main. */
        R_ABORT_UNLESS(::fsStorageRead(std::addressof(storage), 0x4000, work_buffer, sizeof(work_buffer)));

        /* Check for custom public key. */
        if (std::memcmp(work_buffer + bct_pubk_offset, CustomPublicKey, sizeof(CustomPublicKey)) != 0) {
            return false;
        }

        return true;
    }

}
