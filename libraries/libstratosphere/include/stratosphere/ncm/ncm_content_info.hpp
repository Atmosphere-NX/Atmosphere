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
#include <stratosphere/fs/fs_content_attributes.hpp>
#include <stratosphere/ncm/ncm_content_id.hpp>
#include <stratosphere/ncm/ncm_content_type.hpp>

namespace ams::ncm {

    struct ContentInfo {
        static constexpr fs::ContentAttributes DefaultContentAttributes = fs::ContentAttributes_None;

        ContentId content_id;
        u32 size_low;
        u8 size_high;
        u8 content_attributes;
        ContentType content_type;
        u8 id_offset;

        constexpr const ContentId &GetId() const {
            return this->content_id;
        }

        constexpr u64 GetSize() const {
            return (static_cast<u64>(this->size_high) << 32) | static_cast<u64>(this->size_low);
        }

        constexpr fs::ContentAttributes GetContentAttributes() const {
            return static_cast<fs::ContentAttributes>(this->content_attributes & 0xF);
        }

        constexpr ContentType GetType() const {
            return this->content_type;
        }

        constexpr u8 GetIdOffset() const {
            return this->id_offset;
        }

        static constexpr ContentInfo Make(ContentId id, u64 size, fs::ContentAttributes attr, ContentType type, u8 id_ofs = 0) {
            const u32 size_low = size & 0xFFFFFFFFu;
            const u8 size_high = static_cast<u8>(size >> 32);
            return {
                .content_id         = id,
                .size_low           = size_low,
                .size_high          = size_high,
                .content_attributes = attr,
                .content_type       = type,
                .id_offset          = id_ofs,
            };
        }
    };

    static_assert(sizeof(util::is_pod<ContentInfo>::value));
    static_assert(sizeof(ContentInfo)  == 0x18);

}
