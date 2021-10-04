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

        template<typename Handler>
        Result ForEachContentInfo(const ContentMetaKey &key, ncm::ContentMetaDatabase *db, Handler handler) {
            constexpr s32 MaxPerIteration = 0x10;
            ContentInfo info_list[MaxPerIteration];
            s32 offset = 0;
            while (true) {
                /* List the content infos. */
                s32 count;
                R_TRY(db->ListContentInfo(std::addressof(count), info_list, MaxPerIteration, key, offset));

                /* Handle all that we listed. */
                for (s32 i = 0; i < count; i++) {
                    bool done = false;
                    R_TRY(handler(std::addressof(done), info_list[i]));
                    if (done) {
                        break;
                    }
                }

                /* Check if we're done. */
                if (count != MaxPerIteration) {
                    break;
                }

                offset += count;
            }

            return ResultSuccess();
        }

    }

    s64 CalculateRequiredSize(s64 file_size, s64 cluster_size) {
        return file_size + CalculateAdditionalContentSize(file_size, cluster_size);
    }

    s64 CalculateRequiredSizeForExtension(s64 file_size, s64 cluster_size) {
        return file_size + ((file_size / ConcatenationFileSizeMax) + 1) * cluster_size;
    }

    Result EstimateRequiredSize(s64 *out_size, const ContentMetaKey &key, ncm::ContentMetaDatabase *db) {
        s64 size = 0;
        R_TRY(ForEachContentInfo(key, db, [&size](bool *out_done, const ContentInfo &info) -> Result {
            size += CalculateRequiredSize(info.GetSize(), MaxClusterSize);
            *out_done = false;
            return ResultSuccess();
        }));

        *out_size = size;
        return ResultSuccess();
    }

}
