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
#include <stratosphere.hpp>
#include "fssystem_key_slot_cache.hpp"

namespace ams::fssystem {

    namespace {

        Result DecompressLz4(void *dst, size_t dst_size, const void *src, size_t src_size) {
            R_UNLESS(util::DecompressLZ4(dst, dst_size, src, src_size) == static_cast<int>(dst_size), fs::ResultUnexpectedInCompressedStorageC());
            R_SUCCEED();
        }

        constexpr DecompressorFunction GetNcaDecompressorFunction(CompressionType type) {
            switch (type) {
                case CompressionType_Lz4:
                    return DecompressLz4;
                default:
                    return nullptr;
            }
        }

        constexpr NcaCompressionConfiguration g_nca_compression_configuration {
            .get_decompressor = GetNcaDecompressorFunction,
        };

    }

    const ::ams::fssystem::NcaCompressionConfiguration *GetNcaCompressionConfiguration() {
        return std::addressof(g_nca_compression_configuration);
    }

}
