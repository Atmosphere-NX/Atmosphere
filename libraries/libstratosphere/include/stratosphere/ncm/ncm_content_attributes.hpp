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
#include <stratosphere/fs/fs_content_attributes.hpp>

namespace ams::ncm {

    /* TODO: What is this struct, really? It is presumably not ContentAttributes/has more fields. */
    struct ContentAttributes {
        fs::ContentAttributes content_attributes;
        u8 unknown[0xF];

        static constexpr ALWAYS_INLINE ContentAttributes Make(fs::ContentAttributes attr) {
            return { attr, };
        }
    };
    static_assert(util::is_pod<ContentAttributes>::value);
    static_assert(sizeof(ContentAttributes) == 0x10);

    constexpr inline const ContentAttributes DefaultContentAttributes = ContentAttributes::Make(fs::ContentAttributes_None);

}
