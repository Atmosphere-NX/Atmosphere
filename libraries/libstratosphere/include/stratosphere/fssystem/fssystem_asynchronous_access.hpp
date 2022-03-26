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
#include <vapours.hpp>

namespace ams::fssystem {

    /* ACCURATE_TO_VERSION: 13.4.0.0 */
    class IAsynchronousAccessSplitter {
        public:
            static IAsynchronousAccessSplitter *GetDefaultAsynchronousAccessSplitter();
        public:
            constexpr IAsynchronousAccessSplitter() = default;
            constexpr virtual ~IAsynchronousAccessSplitter() { /* ... */ }
        public:
            Result QueryNextOffset(s64 *out, s64 start_offset, s64 end_offset, s64 access_size, s64 alignment_size);
        public:
            virtual Result QueryAppropriateOffset(s64 *out, s64 offset, s64 access_size, s64 alignment_size) = 0;
            virtual Result QueryInvocationCount(s64 *out, s64 start_offset, s64 end_offset, s64 access_size, s64 alignment_size) { AMS_UNUSED(out, start_offset, end_offset, access_size, alignment_size); AMS_ABORT("TODO"); }
    };

    /* ACCURATE_TO_VERSION: 13.4.0.0 */
    class DefaultAsynchronousAccessSplitter final : public IAsynchronousAccessSplitter {
        public:
            constexpr DefaultAsynchronousAccessSplitter() = default;
        public:
            virtual Result QueryAppropriateOffset(s64 *out, s64 offset, s64 access_size, s64 alignment_size) override {
                /* Align the access. */
                *out = util::AlignDown(offset + access_size, alignment_size);
                R_SUCCEED();
            }

            virtual Result QueryInvocationCount(s64 *out, s64 start_offset, s64 end_offset, s64 access_size, s64 alignment_size) override {
                /* Determine aligned access count. */
                *out = util::DivideUp(end_offset - util::AlignDown(start_offset, alignment_size), access_size);
                R_SUCCEED();
            }
    };

    /* ACCURATE_TO_VERSION: 13.4.0.0 */
    inline IAsynchronousAccessSplitter *IAsynchronousAccessSplitter::GetDefaultAsynchronousAccessSplitter() {
        static constinit DefaultAsynchronousAccessSplitter s_default_access_splitter;
        return std::addressof(s_default_access_splitter);
    }

}
