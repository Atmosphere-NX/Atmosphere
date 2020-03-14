/*
 * Copyright (c) 2019-2020 Adubbz, Atmosphère-NX
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

namespace ams::ncm {

    namespace {
        constexpr inline s64 EncryptionMetadataSize   = 16_KB;
        constexpr inline s64 ConcatenationFileSizeMax = 4_GB;

        constexpr s64 CalculateAdditionalContentSize(s64 file_size, s64 cluster_size) {
            /* Account for the encryption header. */
            s64 size = EncryptionMetadataSize;

            /* Account for the file size splitting costs. */
            size += ((file_size / ConcatenationFileSizeMax) + 1) * cluster_size;

            /* Account for various overhead costs. */
            size += cluster_size * 3;

            return size;
        }
        
    }
    
    s64 CalculateRequiredSize(s64 file_size, s64 cluster_size) {
        return file_size + CalculateAdditionalContentSize(file_size, cluster_size);
    }
    
    s64 CalculateRequiredSizeForExtension(s64 file_size, s64 cluster_size) {
        return file_size + ((file_size / ConcatenationFileSizeMax) + 1) * cluster_size;
    }

}
