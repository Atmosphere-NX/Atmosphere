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

namespace ams::ncm {

    struct alignas(4) ContentId {
        util::Uuid uuid;

        bool operator==(const ContentId &other) const {
            return this->uuid == other.uuid;
        }

        bool operator!=(const ContentId &other) const {
            return this->uuid != other.uuid;
        }

        bool operator==(const util::Uuid &other) const {
            return this->uuid == other;
        }

        bool operator!=(const util::Uuid &other) const {
            return this->uuid != other;
        }
    };

    static_assert(alignof(ContentId) == 4);

    constexpr inline ContentId InvalidContentId = { util::InvalidUuid };

}
