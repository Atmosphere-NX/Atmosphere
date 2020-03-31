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
#include <vapours.hpp>
#include <stratosphere/nim/nim_task_id_common.hpp>

namespace ams::nim {

    #define AMS_NIM_DEFINE_TASK_ID(ID, ALIGN)                       \
    struct alignas(ALIGN) ID {                                      \
        util::Uuid uuid;                                            \
                                                                    \
        ALWAYS_INLINE bool IsValid() const {                        \
            return this->uuid != util::InvalidUuid;                 \
        }                                                           \
    };                                                              \
                                                                    \
    static_assert(alignof(ID) == ALIGN);                            \
                                                                    \
    ALWAYS_INLINE bool operator==(const ID &lhs, const ID &rhs) {   \
        return lhs.uuid == rhs.uuid;                                \
    }                                                               \
                                                                    \
    ALWAYS_INLINE bool operator!=(const ID &lhs, const ID &rhs) {   \
        return lhs.uuid != rhs.uuid;                                \
    }                                                               \
                                                                    \
    constexpr inline ID Invalid##ID = { util::InvalidUuid }

}
