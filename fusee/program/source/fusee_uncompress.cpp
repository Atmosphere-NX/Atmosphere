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
#include <exosphere.hpp>
#include "fusee_uncompress.hpp"

namespace ams::nxboot {

    namespace {

        class Lz4Uncompressor {
            private:
                const u8 *m_src;
                size_t m_src_size;
                size_t m_src_offset;
                u8 *m_dst;
                size_t m_dst_size;
                size_t m_dst_offset;
            public:
                Lz4Uncompressor(void *dst, size_t dst_size, const void *src, size_t src_size) : m_src(static_cast<const u8 *>(src)), m_src_size(src_size), m_src_offset(0), m_dst(static_cast<u8 *>(dst)), m_dst_size(dst_size), m_dst_offset(0) {
                    /* ... */
                }

                void Uncompress() {
                    while (true) {
                        /* Read a control byte. */
                        const u8 control = this->ReadByte();

                        /* Copy what it specifies we should copy. */
                        this->Copy(this->GetCopySize(control >> 4));

                        /* If we've exceeded size, we're done. */
                        if (m_src_offset >= m_src_size) {
                            break;
                        }

                        /* Read the wide copy offset. */
                        u16 wide_offset = this->ReadByte();
                        AMS_ABORT_UNLESS(this->CanRead());
                        wide_offset |= (this->ReadByte() << 8);

                        /* Determine the copy size. */
                        const size_t wide_copy_size = this->GetCopySize(control & 0xF);

                        /* Copy bytes. */
                        const size_t end_offset = m_dst_offset + wide_copy_size + 4;
                        for (size_t cur_offset = m_dst_offset; cur_offset < end_offset; m_dst_offset = (++cur_offset)) {
                            AMS_ABORT_UNLESS(wide_offset <= cur_offset);

                            m_dst[cur_offset] = m_dst[cur_offset - wide_offset];
                        }
                    }
                }
            private:
                u8 ReadByte() {
                    return m_src[m_src_offset++];
                }

                bool CanRead() const {
                    return m_src_offset < m_src_size;
                }

                size_t GetCopySize(u8 control) {
                    size_t size = control;

                    if (control >= 0xF) {
                        do {
                            AMS_ABORT_UNLESS(this->CanRead());
                            control = this->ReadByte();
                            size += control;
                        } while (control == 0xFF);
                    }

                    return size;
                }

                void Copy(size_t size) {
                    __builtin_memcpy(m_dst + m_dst_offset, m_src + m_src_offset, size);
                    m_dst_offset += size;
                    m_src_offset += size;
                }
        };

    }

    void Uncompress(void *dst, size_t dst_size, const void *src, size_t src_size) {
        /* Create an execute a decompressor. */
        Lz4Uncompressor(dst, dst_size, src, src_size).Uncompress();
    }

}