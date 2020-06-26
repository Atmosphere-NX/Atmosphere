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
#include <vapours.hpp>
#include <stratosphere/sf/sf_buffer_tags.hpp>

namespace ams::err {

    enum class ErrorContextType : u8 {
        None              = 0,
        Http              = 1,
        FileSystem        = 2,
        WebMediaPlayer    = 3,
        LocalContentShare = 4,
    };

    struct PaddingErrorContext {
        u8 padding[0x200 - 8];
    };

    struct ErrorContext : public sf::LargeData, public sf::PrefersMapAliasTransferMode {
        ErrorContextType type;
        u8 reserved[7];

        union {
            PaddingErrorContext padding;
        };
    };
    static_assert(sizeof(ErrorContext) == 0x200);
    static_assert(util::is_pod<ErrorContext>::value);

}
