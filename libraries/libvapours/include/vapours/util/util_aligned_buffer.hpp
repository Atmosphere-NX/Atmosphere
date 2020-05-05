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

#pragma once
#include <vapours/common.hpp>
#include <vapours/assert.hpp>
#include <vapours/util/util_alignment.hpp>

namespace ams::util {

    template<size_t Alignment, size_t Size>
    class AlignedBuffer {
        private:
            static constexpr size_t AlignedSize = ((Size + Alignment - 1) / Alignment) * Alignment;
            static_assert(AlignedSize % Alignment == 0);
        private:
            u8 buffer[Alignment + AlignedSize];
        public:
            ALWAYS_INLINE operator u8 *() { return reinterpret_cast<u8 *>(util::AlignUp(reinterpret_cast<uintptr_t>(this->buffer), Alignment)); }
    };

}