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

    enum CompressionType {
        CompressionType_None    = 0,
        CompressionType_1       = 1,
        CompressionType_2       = 2,
        CompressionType_Lz4     = 3,
        CompressionType_Unknown = 4,
    };

    using DecompressorFunction    = Result (*)(void *, size_t, const void *, size_t);
    using GetDecompressorFunction = DecompressorFunction (*)(CompressionType);

    namespace CompressionTypeUtility {

        constexpr bool IsBlockAlignmentRequired(CompressionType type) {
            return type != CompressionType_None && type != CompressionType_1;
        }

        constexpr bool IsDataStorageAccessRequired(CompressionType type) {
            return type != CompressionType_1;
        }

        constexpr bool IsRandomAccessible(CompressionType type) {
            return type == CompressionType_None;
        }

        constexpr bool IsUnknownType(CompressionType type) {
            return type >= CompressionType_Unknown;
        }

    }

}
