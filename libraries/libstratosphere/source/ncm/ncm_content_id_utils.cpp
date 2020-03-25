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

        void GetStringFromBytes(char *dst, const void *src, size_t count) {
            for (size_t i = 0; i < count; i++) {
                std::snprintf(dst + 2 * i, 3, "%02x", static_cast<const u8 *>(src)[i]);
            }
        }

        bool GetBytesFromString(void *dst, size_t dst_size, const char *src, size_t src_size) {
            /* Each byte is comprised of hex characters. */
            if (!util::IsAligned(src_size, 2) || (dst_size * 2 < src_size)) {
                return false;
            }

            /* Convert each character pair to a byte until we reach the end. */
            for (size_t i = 0; i < src_size; i += 2) {
                char tmp[3];
                strlcpy(tmp, src + i, sizeof(tmp));

                char *err = nullptr;
                reinterpret_cast<u8 *>(dst)[i / 2] = static_cast<u8>(std::strtoul(tmp, std::addressof(err), 16));
                if (*err != '\x00') {
                    return false;
                }
            }

            return true;
        }

    }

    ContentIdString GetContentIdString(ContentId id) {
        ContentIdString str;
        GetStringFromContentId(str.data, sizeof(str), id);
        return str;
    }

    void GetStringFromContentId(char *dst, size_t dst_size, ContentId id) {
        AMS_ABORT_UNLESS(dst_size > ContentIdStringLength);
        GetStringFromBytes(dst, std::addressof(id), sizeof(id));
    }

    void GetStringFromRightsId(char *dst, size_t dst_size, fs::RightsId id) {
        AMS_ABORT_UNLESS(dst_size > RightsIdStringLength);
        GetStringFromBytes(dst, std::addressof(id), sizeof(id));
    }

    void GetTicketFileStringFromRightsId(char *dst, size_t dst_size, fs::RightsId id) {
        AMS_ABORT_UNLESS(dst_size > TicketFileStringLength);
        ContentIdString str;
        GetStringFromRightsId(str.data, sizeof(str), id);
        std::snprintf(dst, dst_size, "%s.tik", str.data);
    }

    void GetCertificateFileStringFromRightsId(char *dst, size_t dst_size, fs::RightsId id) {
        AMS_ABORT_UNLESS(dst_size > CertFileStringLength);
        ContentIdString str;
        GetStringFromRightsId(str.data, sizeof(str), id);
        std::snprintf(dst, dst_size, "%s.cert", str.data);
    }

    std::optional<ContentId> GetContentIdFromString(const char *str, size_t len) {
        if (len < ContentIdStringLength) {
            return std::nullopt;
        }

        ContentId content_id;
        return GetBytesFromString(std::addressof(content_id), sizeof(content_id), str, ContentIdStringLength) ? std::optional<ContentId>(content_id) : std::nullopt;
    }

}
