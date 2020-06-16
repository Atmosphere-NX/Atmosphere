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
#include <exosphere.hpp>
#include "secmon_loader_uncompress.hpp"

namespace ams::secmon::loader {

    namespace {

        class Lz4Uncompressor {
            private:
                const u8 *src;
                size_t src_size;
                size_t src_offset;
                u8 *dst;
                size_t dst_size;
                size_t dst_offset;
            public:
                Lz4Uncompressor(void *dst, size_t dst_size, const void *src, size_t src_size) : src(static_cast<const u8 *>(src)), src_size(src_size), src_offset(0), dst(static_cast<u8 *>(dst)), dst_size(dst_size), dst_offset(0) {
                    /* ... */
                }

                void Uncompress() {
                    while (true) {
                        /* Read a control byte. */
                        const u8 control = this->ReadByte();

                        /* Copy what it specifies we should copy. */
                        this->Copy(this->GetCopySize(control >> 4));

                        /* If we've exceeded size, we're done. */
                        if (this->src_offset >= this->src_size) {
                            break;
                        }

                        /* Read the wide copy offset. */
                        u16 wide_offset = this->ReadByte();
                        AMS_ABORT_UNLESS(this->CanRead());
                        wide_offset |= (this->ReadByte() << 8);

                        /* Determine the copy size. */
                        const size_t wide_copy_size = this->GetCopySize(control & 0xF);

                        /* Copy bytes. */
                        const size_t end_offset = this->dst_offset + wide_copy_size + 4;
                        for (size_t cur_offset = this->dst_offset; cur_offset < end_offset; this->dst_offset = (++cur_offset)) {
                            AMS_ABORT_UNLESS(wide_offset <= cur_offset);

                            this->dst[cur_offset] = this->dst[cur_offset - wide_offset];
                        }
                    }
                }
            private:
                u8 ReadByte() {
                    return this->src[this->src_offset++];
                }

                bool CanRead() const {
                    return this->src_offset < this->src_size;
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
                    __builtin_memcpy(this->dst + this->dst_offset, this->src + this->src_offset, size);
                    this->dst_offset += size;
                    this->src_offset += size;
                }
        };

    }

    void Uncompress(void *dst, size_t dst_size, const void *src, size_t src_size) {
        /* Create an execute a decompressor. */
        Lz4Uncompressor(dst, dst_size, src, src_size).Uncompress();
    }

}