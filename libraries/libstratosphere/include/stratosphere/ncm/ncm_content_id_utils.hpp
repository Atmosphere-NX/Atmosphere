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
#pragma once
#include <stratosphere/fs/fs_rights_id.hpp>
#include <stratosphere/ncm/ncm_content_id.hpp>
#include <stratosphere/ncm/ncm_content_info_data.hpp>

namespace ams::ncm {

    constexpr inline size_t ContentIdStringLength  = 2 * sizeof(ContentId);
    constexpr inline size_t RightsIdStringLength   = 2 * sizeof(fs::RightsId);
    constexpr inline size_t TicketFileStringLength = RightsIdStringLength + 4;
    constexpr inline size_t CertFileStringLength   = RightsIdStringLength + 5;

    struct ContentIdString {
        char data[ContentIdStringLength + 1];
    };

    ContentIdString GetContentIdString(ContentId id);

    void GetStringFromContentId(char *dst, size_t dst_size, ContentId id);
    void GetStringFromRightsId(char *dst, size_t dst_size, fs::RightsId id);

    void GetTicketFileStringFromRightsId(char *dst, size_t dst_size, fs::RightsId id);
    void GetCertificateFileStringFromRightsId(char *dst, size_t dst_size, fs::RightsId id);

    std::optional<ContentId> GetContentIdFromString(const char *str, size_t len);

}
