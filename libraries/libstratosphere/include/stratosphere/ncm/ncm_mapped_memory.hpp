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
#pragma once
#include <stratosphere.hpp>

namespace ams::ncm {

    struct MappedMemory {
        u64 id;
        size_t offset;
        u8 *buffer;
        size_t buffer_size;

        bool IsIncluded(size_t o, size_t sz) const {
            return this->offset <= o && sz <= this->buffer_size && (o + sz) <= (this->offset + this->buffer_size);
        }

        u8 *GetBuffer(size_t o, size_t sz) const {
            AMS_ASSERT(this->buffer != nullptr);
            AMS_ASSERT(this->IsIncluded(o, sz));
            AMS_UNUSED(sz);

            return this->buffer + (o - this->offset);
        }
    };
    static_assert(util::is_pod<MappedMemory>::value);

    class IMapper {
        public:
            virtual ~IMapper() { /* ... */ }
        public:
            virtual Result GetMappedMemory(MappedMemory *out, size_t offset, size_t size) = 0;
            virtual Result MarkUsing(u64 id) = 0;
            virtual Result UnmarkUsing(u64 id) = 0;
            virtual Result MarkDirty(u64 id) = 0;
        protected:
            virtual Result MapImpl(MappedMemory *out, Span<u8> data, size_t offset, size_t size) = 0;
            virtual Result UnmapImpl(MappedMemory *mem) = 0;
            virtual bool IsAccessibleSizeUpdatable() = 0;
    };

}